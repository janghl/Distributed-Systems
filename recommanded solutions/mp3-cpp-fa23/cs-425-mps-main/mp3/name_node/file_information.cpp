#include <cassert>
#include <bits/stdc++.h>

#include "file_information.h"
#include "utils.hpp"
#include "operation_type.hpp"
#include "request.hpp"
#include "response.hpp"
#include "name_node_block_report_manager.hpp"


// ==== CONSTRUCTORS/OPERATOR= ======
FileInformation& FileInformation::operator=(FileInformation&& other) {
	move_helper(other);
	return *this;
}

FileInformation::FileInformation(FileInformation&& other) {
        move_helper(other);
}

void FileInformation::move_helper(FileInformation& other) {
	file_name = std::move(other.file_name);
	update_counter = std::move(other.update_counter);
	replica_ids = std::move(other.replica_ids);
	request_manager = std::move(other.request_manager);
    block_manager = other.block_manager;
    last_committed_version = other.last_committed_version;
    tombstone = other.tombstone;
}

FileInformation::FileInformation(const std::string& new_file_name, bool generate_replicas, BlockManager* block_manager) 
    : file_name(new_file_name), update_counter(0), replica_ids(), request_manager(), block_manager(block_manager),
        last_committed_version(0), tombstone(false) {
        if (generate_replicas) {
            _generate_replica_machineIDs();
        }
}

FileInformation& FileInformation::operator=(const FileInformation& other) {
    if (this != &other) {
        this->file_name = other.get_file_name(); 	
        this->update_counter = other._get_update_counter(); 
        this->replica_ids = other.get_replica_ids();   
        this->block_manager = other.block_manager;
        this->tombstone = other.tombstone;
        this->last_committed_version = other.last_committed_version;
    }
    return *this;
}

// ==== GETTERS/SETTERS ======

void FileInformation::lock() {
    mtx.lock();
}

void FileInformation::unlock() {
    mtx.unlock();
}

std::string FileInformation::get_file_name() const {
    return this->file_name;
}

uint32_t FileInformation::_get_update_counter() const {
    return this->update_counter;
}

void FileInformation::_set_update_counter(uint32_t new_update_counter) {
    if (new_update_counter < update_counter) {
        return;
    }
    update_counter = new_update_counter;
}

void FileInformation::set_update_counter(uint32_t new_update_counter) {
    std::lock_guard<std::mutex> lock(mtx);
    _set_update_counter(new_update_counter);
}

uint32_t FileInformation::_get_last_committed_version() const {
    return last_committed_version;
}

void FileInformation::_set_last_committed_version(uint32_t new_last_committed_version) {
    last_committed_version = new_last_committed_version;
}

void FileInformation::set_last_committed_version(uint32_t new_last_committed_version) {
    std::lock_guard<std::mutex> lock(mtx);
    if (new_last_committed_version < last_committed_version) {
        return;
    }
    _set_last_committed_version(new_last_committed_version);
}

RequestQueues& FileInformation::get_request_manager() {
    return request_manager;
}

// ==== REPLICA HANDLING ======

//generate default replica ids
void FileInformation::_generate_replica_machineIDs(bool lock_membership) {
    uint32_t retry_count = 0;

    int replicas[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::shuffle(replicas, replicas + 10, std::default_random_engine(time(NULL)));

    uint32_t replica_index = 0;
    while (replica_ids.size() < NUM_REPLICAS && replica_index < 10) {
        uint32_t machine_id = replicas[replica_index];
	    ++replica_index;

        #ifndef TEST_MODE
            if(membership_list_manager.has_not_failed(machine_id, lock_membership) && replica_ids.find(machine_id) == replica_ids.end()) {
                replica_ids.insert(machine_id);
            } else {
                retry_count++;
            }
        #else
            // Code that should run in test mode
            replica_ids.insert(machine_id);
        #endif   
    }   
    if (replica_index >= 10) {
	    std::cerr << "Error: Could not find enough alive machines to generate " << NUM_REPLICAS << " replicas." << std::endl;
    }
}

const std::unordered_set<uint32_t>& FileInformation::get_replica_ids() const {
    return this->replica_ids;
}

bool FileInformation::_contains_replica(uint32_t replica_id) const {
    return replica_ids.find(replica_id) != replica_ids.end();
}

// Method to add a replica
void FileInformation::_add_replica(uint32_t replica_id) {
    (this->replica_ids).insert(replica_id);
}

// delete a replica
void FileInformation::_delete_replica(uint32_t replica_id) {
    for (auto it = replica_ids.begin(); it != replica_ids.end(); ++it) {
        if (*it == replica_id) {
            replica_ids.erase(it);
            break;
        }
    }
}

bool FileInformation::is_tombstone() {
    std::lock_guard<std::mutex> lock(mtx);
    return tombstone;
}

// ==== NEW REQUEST HANDLING ======


void FileInformation::_set_request_fields(uint32_t client_fd, Request& req) {
    Metadata* meta;
    if (req.get_request_type() == OperationType::WRITE) {
        meta = new WriteMetadata(client_fd, false);
        req.set_update_counter(++update_counter);
    } else if (req.get_request_type() == OperationType::READ) {
        meta = new ReadMetadata(client_fd);
    } else {
        meta = new Metadata(client_fd);
    }
    req.set_metadata(meta);
}

void FileInformation::set_request_fields(uint32_t client_fd, Request& req) {
    std::lock_guard<std::mutex> lock(mtx);
    _set_request_fields(client_fd, req);
}

void FileInformation::manage_request_queue(Request& req) {
    if (req.get_request_type() == OperationType::LS) {
#ifdef DEBUG_MP3_FILE_INFORMATION
        std::cout << "NAME_NODE_FILE_INFORMATION: " << "detected LS request" << std::endl;
#endif
        handle_ls_request(req);
        return;
    }
    request_manager.register_new_request(req);

    if (request_manager.can_try_scheduling_requests()) {
        schedule_to_datanode(request_manager.get_current_running_request_type());
    }
}

// ==== EXECUTING REQUEST ======

void FileInformation::handle_ls_request(Request& current_request) {
    std::lock_guard<std::mutex> lock(mtx);
    Metadata* current_request_metadata = current_request.get_metadata();
    if (this->tombstone) { // file has been deleted!
        file_not_found(current_request_metadata->client_socket, current_request.get_file_name(), OperationType::LS);
        close(current_request_metadata->client_socket);
        return;
    }


    LSResponse response(membership_list_manager.get_self_id(), OperationType::LS, 0, current_request.get_file_name(), this->replica_ids);
#ifdef DEBUG_MP3_FILE_INFORMATION
    std::cout << "NAME_NODE_FILE_INFORMATION: " << "sending response to LS request" << std::endl;
    std::cout << "NAME_NODE_FILE_INFORMATION: " << "there are " << this->replica_ids.size() << " replicas for file " << current_request.get_file_name() << ":";
    for (const uint32_t i : this->replica_ids) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
#endif
    send_response_over_socket(current_request_metadata->client_socket, response, "NAME_NODE_FILE_INFORMATION: ");
    close(current_request_metadata->client_socket);
}

void FileInformation::handle_response(std::shared_ptr<Request> current_request, int data_node_server_socket, uint32_t data_node_id) {
    Metadata* current_request_metadata = current_request->get_metadata();
    bool request_part_of_read_repair = current_request->get_request_type() == OperationType::WRITE && reinterpret_cast<WriteMetadata*>(current_request_metadata)->read_repair == true;
#ifdef DEBUG_MP3_FILE_INFORMATION
    std::cout << "NAME_NODE_FILE_INFORMATION: " << current_request->get_file_name() << " trying to handle response for request id " << current_request_metadata->request_id 
                << " for data node " << data_node_id << " on socket " << data_node_server_socket << std::endl;
#endif

    uint32_t payload_size;
    Response* response = nullptr;
    if (data_node_server_socket >= 0) {
        response = get_response_over_socket(data_node_server_socket, payload_size);
        close(data_node_server_socket);
    }

#ifdef DEBUG_MP3_FILE_INFORMATION
    std::cout << "NAME_NODE_FILE_INFORMATION: " << current_request->get_file_name() << " got response from data node " << data_node_id << " for request id" << current_request_metadata->request_id << std::endl;
#endif

    bool is_request_complete = false;
    bool respond_to_client = false;

    if (response == nullptr) {
#ifdef DEBUG_MP3_FILE_INFORMATION
	    std::cout << "NAME_NODE_FILE_INFORMATION: " << current_request->get_file_name() << " Did not get valid response from a data node server; data node died?" << std::endl;
#endif
    }

    // handle writes part of read repair
    if (request_part_of_read_repair) {
        respond_to_client = false;
        current_request_metadata->lock();
	    WriteMetadata* write_meta = reinterpret_cast<WriteMetadata*>(current_request_metadata);
#ifdef DEBUG_MP3_FILE_INFORMATION
        std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " got a response on read repair from data node " << data_node_id << std::endl;
        std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " need to hear from " <<  write_meta->read_repair_on_replicas.size() << " nodes: ";
        for (const uint32_t i : write_meta->read_repair_on_replicas) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
#endif
        if (write_meta->read_repair_on_replicas.find(data_node_id) != write_meta->read_repair_on_replicas.end()) {
#ifdef DEBUG_MP3_FILE_INFORMATION
            std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " Heard from " << (write_meta->num_read_replica_responses+1) << " replicas; need to hear from " << 
                write_meta->read_repair_on_replicas.size() << " in total" << std::endl;
#endif
            is_request_complete = (++write_meta->num_read_replica_responses == write_meta->read_repair_on_replicas.size());
        } else if (write_meta->read_repair_on_replicas.size() == 0) {
            is_request_complete = true;
        }
        current_request_metadata->unlock();
    }

    // record response in request metadata
    current_request_metadata->lock();
    uint32_t count_ACK = ++current_request_metadata->count_ACKs;
    current_request_metadata->data_transfer += payload_size;

    if (response && current_request->get_request_type() != OperationType::READ) {
        if (current_request_metadata->response_to_return == nullptr || response->get_status_code() < 0) {
            if (current_request_metadata->response_to_return != nullptr) {
                delete current_request_metadata->response_to_return;
            }
            current_request_metadata->response_to_return = response;
        }

    } else if (current_request->get_request_type() == OperationType::READ && (response == nullptr || response->get_status_code() < 0)) { // encountered error; must issue read repair if any other node response with successful read
#ifdef DEBUG_MP3_FILE_INFORMATION
        std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " got error on read request " << std::endl;
#endif
        ReadMetadata* read_request_metadata = reinterpret_cast<ReadMetadata*>(current_request_metadata);
        read_request_metadata->read_repair_needed = true;
        read_request_metadata->responses.try_emplace(data_node_id, -1);

    } else if (response && current_request->get_request_type() == OperationType::READ) { // read successful; track info needed to determine if read repair is required and if yes on which nodes
#ifdef DEBUG_MP3_FILE_INFORMATION
        std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " got successful response on read request " << std::endl;
#endif
        ReadMetadata* read_request_metadata = reinterpret_cast<ReadMetadata*>(current_request_metadata);
        WriteSuccessReadResponse* read_response = reinterpret_cast<WriteSuccessReadResponse*>(response);
        if (read_request_metadata->highest_update_counter < read_response->get_update_counter()) {
            read_request_metadata->highest_update_counter = read_response->get_update_counter();
            if (read_request_metadata->responses.size() != 0) {
                // we must do read-repair at the data node that sent response recorded in read_request_metadata->responses have an older version
                // of data than the data node that just responded
                read_request_metadata->read_repair_needed = true;
            }
            // set the response to return to current response assuming we haven't logged an error response yet
            if (read_request_metadata->response_to_return == nullptr || read_request_metadata->response_to_return->get_status_code() >= 0) { 
                if (read_request_metadata->response_to_return != nullptr) {
                    delete read_request_metadata->response_to_return;
                }
                read_request_metadata->response_to_return = read_response;
            }
            read_request_metadata->read_repair_source = read_response;
        }
        read_request_metadata->responses.try_emplace(data_node_id, read_response->get_update_counter());
    }
    // TODO: currently if any of the first X responses has an error (status code), we send the client the error response. Ideally, what we should do is wait a while
    // and check if any of the other nodes (that haven't responsed yet) send a successful response and if yes, we should ignore the error status. However, handling this
    // behaviour is complicated and for now, since we only have 4 replicas and X=4, this is not needed.
    current_request_metadata->unlock();
#ifdef DEBUG_MP3_FILE_INFORMATION
    std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " got acks from " << count_ACK << " replicas on request " << current_request_metadata->request_id
	    << std::endl;
#endif

    if (!request_part_of_read_repair) {
        is_request_complete = (count_ACK == X);
        respond_to_client = is_request_complete;
    }

#ifdef DEBUG_MP3_FILE_INFORMATION
    std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " request id " << current_request_metadata->request_id << " is complete? " << is_request_complete << 
                " and sending response to client " << respond_to_client << std::endl;
#endif

    // if request is complete, acknowledge the completion to client
    if (is_request_complete && respond_to_client) {
        // send ACK to data node
        if (current_request_metadata->response_to_return == nullptr) { // error
            if (current_request->get_request_type() != OperationType::WRITE) {
                Response response(self_machine_number, current_request->get_request_type(), FATAL_DATA_NODE_CRASH_QUORUM_IMPOSSIBLE, current_request->get_file_name());
                send_response_over_socket(current_request_metadata->client_socket, response, "NAME_NODE_FILE_INFORMATION: ");
                close(current_request_metadata->client_socket);
            } else {
                WriteSuccessReadResponse write_response(self_machine_number, current_request->get_request_type(), FATAL_DATA_NODE_CRASH_QUORUM_IMPOSSIBLE, 
                        current_request->get_file_name(), current_request->get_update_counter());
                send_response_over_socket(current_request_metadata->client_socket, write_response, "NAME_NODE_FILE_INFORMATION: ");
                close(current_request_metadata->client_socket);
            }
        } else if (dynamic_cast<WriteSuccessReadResponse*>(current_request_metadata->response_to_return)) { // error or success - read or write
            send_response_over_socket(current_request_metadata->client_socket, *reinterpret_cast<WriteSuccessReadResponse*>(current_request_metadata->response_to_return), 
		        "NAME_NODE_FILE_INFORMATION: ");
            handle_ls_request(*current_request);
        } else { // error or success - delete or ls
            send_response_over_socket(current_request_metadata->client_socket, *current_request_metadata->response_to_return, "NAME_NODE_FILE_INFORMATION: ");
            close(current_request_metadata->client_socket);
        }
    }


    // if write request is complete, but failed, check if the file needs to be removed. If the write request succeed, update last committed version
    if (is_request_complete && current_request->get_request_type() == OperationType::WRITE) {
        mtx.lock();
        if (current_request_metadata->response_to_return == nullptr || current_request_metadata->response_to_return->get_status_code() < 0) { // write failed
            if (_get_last_committed_version() == 0) { // no write on this file as completed; so no data node has this file
                if (_get_update_counter() <= current_request->get_update_counter()) { // there are no pending writes on this file
                    this->tombstone = true;
                }
            }
        } else { // write succeed
            _set_last_committed_version(current_request->get_update_counter());
            this->tombstone = false;
        }
        mtx.unlock();
    }

    // delete file information
    if (is_request_complete && current_request->get_request_type() == OperationType::DELETE) {
        mtx.lock();
        _set_last_committed_version(0);
        this->tombstone = true;
        mtx.unlock();
    }

    if (is_request_complete) {
        request_manager.current_request_complete(current_request_metadata->request_id);
    }

    // check we have to issue read repair
    bool execute_read_repair = (count_ACK >= X && current_request->get_request_type() == OperationType::READ && 
                            reinterpret_cast<ReadMetadata*>(current_request_metadata)->read_repair_needed && 
                            reinterpret_cast<ReadMetadata*>(current_request_metadata)->read_repair_source != nullptr); 
#ifdef DEBUG_MP3_FILE_INFORMATION
    std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " execute_read_repair: " << execute_read_repair << " X:" << X << " count_ACK:" << count_ACK << " request type:" << 
            operation_to_string(current_request->get_request_type()) << " read repair needed: " << 
            reinterpret_cast<ReadMetadata*>(current_request_metadata)->read_repair_needed << std::endl;
#endif
        // Note: there is a corner case where multiple reads will execute a read repair because we check count_ACK >= X, but I am hoping that doesn't happen too often
        // and so we don't need to worry about it
    if (execute_read_repair) {
        handle_read_repair(current_request);
    }

    // schedule the next request
    if (is_request_complete) {
        schedule_to_datanode(current_request->get_request_type());
    }
    // cleanup
    if (current_request_metadata->response_to_return != response) {
        delete response;
    }

    // Note: if count_ACK never reaches X, it signals a data node failure. The NameNode failure handler should deal with this case and restart all currently
    // re-running requests including this one. TODO: The NameNode failure handler will somehow kill/disable any running threads or stop them from processing any 
    // responsees they get.
}

// ==== READ REPAIR REQUEST ======

std::string write_read_response_to_file(WriteSuccessReadResponse* response) {
    std::string file_name = response->get_file_name() + ":" + std::to_string(response->get_update_counter());
    std::ofstream file(file_name, std::ofstream::out | std::ofstream::trunc);
#ifdef DEBUG_MP3_FILE_INFORMATION
	std::cout << "NAME_NODE_FILE_INFORMATION: " << "Copying read response into file: " << file_name << std::endl;
#endif
	if (!file) {
		std::cout << "NAME_NODE_FILE_INFORMATION: " << "Could not open file for writing: " << file_name << std::endl;
		std::perror("ofsteam open failed");
		return "DEADBEEF";
	}

    file.write(response->get_data(), response->get_data_length());
	file.close();
    return file_name;
}

void FileInformation::handle_read_repair(std::shared_ptr<Request>& request) {
    ReadMetadata* read_request_metadata = reinterpret_cast<ReadMetadata*>(request->get_metadata());
    WriteSuccessReadResponse* best_response = read_request_metadata->read_repair_source;

    if (best_response == nullptr) {
        return;
    }

    WriteMetadata* write_meta = new WriteMetadata(true);
    for (auto it = read_request_metadata->responses.begin(); it != read_request_metadata->responses.end(); ++it) {
        if (it->second == read_request_metadata->highest_update_counter) {
            continue;
        }
        write_meta->read_repair_on_replicas.insert(it->first);
    }

    if (write_meta->read_repair_on_replicas.size() == 0) {
        delete write_meta;
        return;
    }

    write_meta->set_local_file_name(write_read_response_to_file(best_response));

    Request read_repair_request(OperationType::WRITE, request->get_file_name(), best_response->get_data_length(), 
                                    read_request_metadata->highest_update_counter, write_meta);
    
    write_meta->data_transfer = (read_repair_request.struct_serialized_size() * read_request_metadata->responses.size()); // measure outgoing data transfer for the request

    request_manager.register_read_repair_request(read_repair_request);

    if (request_manager.can_try_scheduling_requests()) {
        schedule_to_datanode(request_manager.get_current_running_request_type());
    }

    delete best_response;
}

// ==== SCHEDULING REQUEST ======

void FileInformation::start_replica_threads(std::shared_ptr<Request> current_request, std::unordered_map<int, int> replica_sockets) {
    Metadata* current_request_metadata = current_request->get_metadata();
    for (auto it = replica_sockets.begin(); it != replica_sockets.end(); ++it) {
#ifdef DEBUG_MP3_FILE_INFORMATION
        std::cout << "NAME_NODE_FILE_INFORMATION: " << "Sending request (file " << current_request->get_file_name() << ") to data node id " << it->first << " on socket " << it->second << std::endl;
#endif
        current_request_metadata->threads.push_back(std::thread(&FileInformation::handle_response, this, current_request, it->second, it->first));
        current_request_metadata->threads.back().detach(); // detach so the thread can run independently of the main thread
    }
}

std::unordered_set<int> FileInformation::get_destination_sockets(const std::unordered_map<int, int>& replica_sockets) const {
    std::unordered_set<int> destination_sockets;
    for (auto it = replica_sockets.begin(); it != replica_sockets.end(); ++it) {
        if (it->second < 0) {
            continue;
        }
        destination_sockets.insert(it->second);
    }
    return destination_sockets;
}

void FileInformation::set_replica_sockets_to_invalid(std::unordered_map<int, int>& replica_sockets) const {
    for (auto it = replica_sockets.begin(); it != replica_sockets.end(); ++it) {
        if (it->second >= 0) {
            close(it->second);
        }
        it->second = -1;
    }
}

void FileInformation::send_request_data_from_socket_to_others(std::shared_ptr<Request> request, std::unordered_map<int, int> replica_sockets) {
    std::unordered_set<int> destination_sockets = get_destination_sockets(replica_sockets);
    if (destination_sockets.size() < X || 
            !send_request_data_from_socket(destination_sockets, *request, "NAME_NODE_FILE_INFORMATION: ", destination_sockets.size() - X + 1)) {
        std::cout << "NAME_NODE_FILE_INFORMATION: send_request_data_from_socket_to_others: " << "Could NOT forward request data (for request on file " << 
                        request->get_file_name() << ") to other sockets" << std::endl;   
        set_replica_sockets_to_invalid(replica_sockets);
    } else {
#ifdef DEBUG_MP3_FILE_INFORMATION
        std::cout << "NAME_NODE_FILE_INFORMATION: send_request_data_from_socket_to_others: " << std::this_thread::get_id() << " Completed forwarding request data (for request on file " << 
                        request->get_file_name() << ") to other sockets" << std::endl;
#endif
    }
    start_replica_threads(request, replica_sockets);
}

void FileInformation::send_request_data_from_file_to_others(std::shared_ptr<Request> request, std::unordered_map<int, int> replica_sockets) {
    std::unordered_set<int> destination_sockets = get_destination_sockets(replica_sockets);
    if (destination_sockets.size() < X || 
            !send_request_data_from_file(destination_sockets, *request, "NAME_NODE_FILE_INFORMATION: ", -1)) {
        std::cout << "NAME_NODE_FILE_INFORMATION: send_request_data_from_file_to_others: " << "Could NOT forward request data (for request on file " << 
                        request->get_file_name() << ") to other sockets" << std::endl;   
        set_replica_sockets_to_invalid(replica_sockets);
    } else {
#ifdef DEBUG_MP3_FILE_INFORMATION
        std::cout << "NAME_NODE_FILE_INFORMATION: send_request_data_from_file_to_others: " << std::this_thread::get_id() << " Completed forwarding request data (for request on file " << 
                        request->get_file_name() << ") to other sockets" << std::endl;
#endif
    }
    start_replica_threads(request, replica_sockets);
}

void FileInformation::schedule_to_datanode(OperationType past_operation) {
    int32_t current_request_id = request_manager.get_next_schedulable_request(past_operation);
    if (current_request_id < 0) {
#ifdef DEBUG_MP3_FILE_INFORMATION
	    std::cout << "NAME_NODE_FILE_INFORMATION: " << "No request schedule at the moment" << std::endl;
#endif
        return;
    }
    send_request_to_datanode(current_request_id);
}

void FileInformation::send_request_to_datanode(uint32_t current_request_id) {
    std::lock_guard<std::mutex> lock(mtx);

    std::shared_ptr<Request> current_request = request_manager.get_running_request(current_request_id);
    Metadata* current_request_metadata = current_request->get_metadata();

    if (current_request->get_request_type() == OperationType::READ && this->tombstone) { // file has been deleted!
        file_not_found(current_request_metadata->client_socket, current_request->get_file_name(), OperationType::READ);
        close(current_request_metadata->client_socket);
        request_manager.current_request_complete(current_request_metadata->request_id);
        schedule_to_datanode(current_request->get_request_type());
        return;
    }

    // send request to all replicas
    std::unordered_set<uint32_t> replica_ids = get_replica_ids();
    std::unordered_map<int, int> replica_sockets; // key = replica id, value = socket for replica
    for(const auto datanode_id : replica_ids) {
        int socket = create_socket_to_datanode(data_node_server_port, datanode_id);
        if (socket > 0) {
            if (send_request_over_socket(socket, *current_request, "NAME_NODE_FILE_INFORMATION: ", nullptr)) {
            } else {
                socket = -1;
            }
        }
#ifdef DEBUG_MP3_FILE_INFORMATION
        std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " - assigning node " << datanode_id << " socket " << socket << std::endl;
#endif
        replica_sockets.try_emplace(datanode_id, socket);
    }

#ifdef DEBUG_MP3_FILE_INFORMATION
	    std::cout << "NAME_NODE_FILE_INFORMATION: file " << current_request->get_file_name() << " Request type is " << static_cast<uint32_t>(current_request->get_request_type()) 
                    << " which same as write? write=" << static_cast<uint32_t>(OperationType::WRITE) << std::endl;
#endif
    if (current_request->get_request_type() == OperationType::WRITE && reinterpret_cast<WriteMetadata*>(current_request_metadata)->read_repair != true) {
#ifdef DEBUG_MP3_FILE_INFORMATION
	    std::cout << "NAME_NODE_FILE_INFORMATION: " << std::this_thread::get_id() << " Sending request data from client socket for file " << 
                        current_request->get_file_name()  << std::endl;
#endif
        std::thread send_data_thread = std::thread(&FileInformation::send_request_data_from_socket_to_others, this, current_request, replica_sockets);
        send_data_thread.detach(); // detach so the thread can run independently of the main thread
    } else if (current_request->get_request_type() == OperationType::WRITE && reinterpret_cast<WriteMetadata*>(current_request_metadata)->read_repair == true) {
#ifdef DEBUG_MP3_FILE_INFORMATION
	    std::cout << "NAME_NODE_FILE_INFORMATION: " << std::this_thread::get_id() << " Sending request data from local file for file " << current_request->get_file_name() 
                    << std::endl;
#endif
        std::thread send_data_thread = std::thread(&FileInformation::send_request_data_from_file_to_others, this, current_request, replica_sockets);
        send_data_thread.detach(); // detach so the thread can run independently of the main thread
    } else {
        start_replica_threads(current_request, replica_sockets);
    }

    // NOTE: we cannot wait for these threads to join right here cause that will block; we just have detach threads so they can run indepdently and never
    // end up calling join on them
}