#pragma once
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <queue>

#ifndef TEST_MODE
#include "../../mp2/membership_list_manager.hpp"
#include "../utils.hpp"
#endif
#include <unistd.h>

#include "request.hpp"

class BlockManager; // forward decleration

extern const int NUM_REPLICAS;
extern const int X;

using n_time = std::chrono::high_resolution_clock;

class RequestQueues {
    std::queue<Request> read_request_queue;     // pending read requests                    -- have third highest priority when scheduling
    std::queue<Request> write_request_queue;    // pending write/delete requests            -- have third highest priority when scheduling
    std::queue<Request> read_repair_queue;      // pending read repairs (write requests)    -- have second highest priority when scheduling
    std::queue<Request> replica_repair_queue;   // pending read requests used when a file has fewer than required replicas -- have highest priority when scheduling

    uint32_t total_operations_registered;       // total operations ever registered on the file during its entire lifetime

    uint32_t num_consecutive_reads;
    uint32_t num_consecutive_writes;

    uint32_t num_reads_running;                 // number of reads currently scheduled ( = 0, 1, or 2)
    uint32_t num_writes_running;                // number of writes currently scheduled ( = 0 or 1)
    std::mutex mtx;                             // mutex to guard concurrent access to this class

    std::vector<std::shared_ptr<Request>> current_request;

    int32_t _setup_request_for_scheduling(Request& request_to_schedule) {
        request_to_schedule.get_metadata()->request_scheduling_time = n_time::now();
        int32_t request_to_schedule_id = request_to_schedule.get_metadata()->request_id;
        current_request.emplace_back(std::make_shared<Request>(std::move(request_to_schedule)));
#ifdef DEBUG_MP3_REQUEST_QUEUES
        std::cout << "NAME_NODE_REQUEST_QUESES: " << "Running requests: #reads: " << num_reads_running << " #writes:" << num_writes_running << std::endl;
#endif
        return request_to_schedule_id;
    }

    int32_t schedule_read_request() {
        num_consecutive_writes = 0;
        ++num_consecutive_reads;
	    ++num_reads_running;

        int32_t request_to_schedule_id = _setup_request_for_scheduling(read_request_queue.front());
        read_request_queue.pop();

#ifdef DEBUG_MP3_REQUEST_QUEUES
	    std::cout << "NAME_NODE_REQUEST_QUEUES: " << "scheduling read request with id " << request_to_schedule_id << std::endl;
#endif
        return request_to_schedule_id;
    }

    int32_t schedule_write_request() {
        num_consecutive_reads = 0;
        ++num_consecutive_writes;
	    ++num_writes_running;

        int32_t request_to_schedule_id = _setup_request_for_scheduling(write_request_queue.front());
        write_request_queue.pop();

#ifdef DEBUG_MP3_REQUEST_QUEUES
	    std::cout << "NAME_NODE_REQUEST_QUEUES: " << "scheduling write request with id " << request_to_schedule_id << std::endl;
#endif
        return request_to_schedule_id;
    }

    int32_t schedule_read_repair_request() {
        // NOTE: We count the read repair as a READ even though it is a technically a WRITE because we do not want to block a parallel client read request.
        // (according to this post: https://piazza.com/class/ll3vjcrbvnt28a/post/889, read repair should not block other requests and this change. Note that 
        // read repair will still block other writes, but there is no way around that)
        num_consecutive_writes = 0;
        ++num_consecutive_reads;
	    ++num_reads_running;

        int32_t request_to_schedule_id = _setup_request_for_scheduling(read_repair_queue.front());
        read_repair_queue.pop();

#ifdef DEBUG_MP3_REQUEST_QUEUES
	    std::cout << "NAME_NODE_REQUEST_QUEUES: " << "scheduling read repair request with id " << request_to_schedule_id << std::endl;
#endif
        return request_to_schedule_id;
    }

    int32_t schedule_replica_repair_request() {
        num_consecutive_writes = 0;
        ++num_consecutive_reads;
	    ++num_reads_running;

        int32_t request_to_schedule_id =  _setup_request_for_scheduling(replica_repair_queue.front());
        replica_repair_queue.pop();
        
#ifdef DEBUG_MP3_REQUEST_QUEUES
	    std::cout << "NAME_NODE_REQUEST_QUEUES: " << "scheduling replica repair request with id " << request_to_schedule_id << std::endl;
#endif
        return request_to_schedule_id;
    }

    std::string stringify_ms_since_epoch(const std::chrono::time_point<n_time>& time) {
       return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count()) + "ms";
    }

    void _register_new_request(Request& request, std::string request_type) {
        request.get_metadata()->request_id = ++total_operations_registered;
        request.get_metadata()->request_arrival_time = n_time::now();

        std::cout << "NAME_NODE_REQUEST_QUESES: " << "Registering a " << request_type << " request; assigning id " << request.get_metadata()->request_id 
            << ". Request sdfs file " << request.get_file_name() << " and operation " << operation_to_string(request.get_request_type()) 
            << std::endl;
    }

public:
    RequestQueues() : num_consecutive_reads(0), num_consecutive_writes(0), total_operations_registered(0), num_reads_running(0), num_writes_running(0) {}

    RequestQueues(RequestQueues&& other) : 
        read_request_queue(std::move(other.read_request_queue)),
        write_request_queue(std::move(other.write_request_queue)),
        num_consecutive_reads(other.num_consecutive_reads), 
        num_consecutive_writes(other.num_consecutive_reads), 
        total_operations_registered(other.num_consecutive_reads), 
        num_reads_running(other.num_consecutive_reads), 
        num_writes_running(other.num_consecutive_reads),
        current_request(std::move(other.current_request)) {}

    RequestQueues& operator=(RequestQueues&& other) {
        read_request_queue = std::move(other.read_request_queue);
        write_request_queue = std::move(other.write_request_queue);
        num_consecutive_reads = other.num_consecutive_reads; 
        num_consecutive_writes = other.num_consecutive_reads;
        total_operations_registered = other.num_consecutive_reads;
        num_reads_running = other.num_consecutive_reads; 
        num_writes_running = other.num_consecutive_reads;
        current_request = std::move(other.current_request);
        return *this;
    }

    void current_request_complete(int32_t request_id) {
        std::lock_guard<std::mutex> lock(mtx);
        uint32_t request_index = 0;
        for (int i = 0; i < current_request.size(); ++i) {
            if (current_request[i]->get_metadata()->request_id == request_id) {
                request_index = i;
                break;
            }
        }
        if (num_reads_running > 0) {
            bool request_part_of_read_repair = current_request[request_index]->get_request_type() == OperationType::WRITE && 
                                                (reinterpret_cast<WriteMetadata*>(current_request[request_index]->get_metadata())->read_repair == true);
            assert(current_request[request_index]->get_request_type() == OperationType::READ || request_part_of_read_repair);
            --num_reads_running;
        } else {
            assert(num_writes_running == 1 && (current_request[request_index]->get_request_type() == OperationType::WRITE ||
                current_request[request_index]->get_request_type() == OperationType::DELETE));
            --num_writes_running;
        }
        assert(num_writes_running >= 0 && num_reads_running >= 0);
        const Metadata* current_request_metadata = current_request[request_index]->get_metadata();
        const auto request_completion_time = n_time::now();
        auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(request_completion_time - current_request_metadata->request_scheduling_time).count();
        auto arrival_to_completion = std::chrono::duration_cast<std::chrono::milliseconds>(request_completion_time - current_request_metadata->request_arrival_time).count();

        std::cout << "NAME_NODE_REQUEST_QUESES: file " << current_request[request_index]->get_file_name() << " Request id " << request_id << " completed" << std::endl
                    << "Arrival time: " << stringify_ms_since_epoch(current_request_metadata->request_arrival_time) << std::endl
                    << "Scheduling time: " << stringify_ms_since_epoch(current_request_metadata->request_scheduling_time) << std::endl
                    << "Completion time: " << stringify_ms_since_epoch(request_completion_time) << "ms"  << std::endl
                    << "Execution time: " << execution_time << "ms"  << std::endl
                    << "Arrival to completion time: " << arrival_to_completion << "ms" << std::endl 
                    << "In+Out Data Transfer: " << current_request_metadata->data_transfer << "bytes" << std::endl
                    << "In+Out Bandwidth: " << double(current_request_metadata->data_transfer) / (double(execution_time) / 1000.0) << "bytes/sec" << std::endl;

#ifdef DEBUG_MP3_REQUEST_QUEUES
        std::cout << "NAME_NODE_REQUEST_QUESES: " << "Running requests: #reads: " << num_reads_running << " #writes:" << num_writes_running << std::endl;
#endif
        current_request.erase(current_request.begin() + request_index);
    }

    OperationType get_current_running_request_type() {
        std::lock_guard<std::mutex> lock(mtx);
        if (current_request.size() == 0) {
            return OperationType::INVALID;
        }
        return current_request[0]->get_request_type();
    }

    // Inserts request into appropriate queue and takes complete ownership over the request. Namely, the input request object is destroyed so the caller
    // cannot modify/access the request after registereing it.
    void register_new_request(Request& request) {
        std::lock_guard<std::mutex> lock(mtx);
        _register_new_request(request, "client");
        if (request.get_request_type() == OperationType::READ) {
            read_request_queue.emplace(std::move(request));
            return;
        }
        write_request_queue.emplace(std::move(request));
    }

    void register_read_repair_request(Request& request) {
        std::lock_guard<std::mutex> lock(mtx);
        _register_new_request(request, "read repair");
        assert(request.get_request_type() == OperationType::WRITE);
        read_repair_queue.emplace(std::move(request));
    }

    void register_replica_repair_request(Request& request) {
        std::lock_guard<std::mutex> lock(mtx);
        _register_new_request(request, "replica repair");
        assert(request.get_request_type() == OperationType::READ);
        replica_repair_queue.emplace(std::move(request));
    }

    bool can_try_scheduling_requests() {
        std::lock_guard<std::mutex> lock(mtx);
        if (num_reads_running == 2 || num_writes_running == 1) {
            return false;
        }
        return true;
    }

    std::shared_ptr<Request> get_running_request(int32_t request_id) {
        for (int i = 0; i < current_request.size(); ++i) {
            if (current_request[i]->get_metadata()->request_id == request_id) {
                return current_request[i];
            }
        }
        return nullptr;
    }

    int32_t get_next_schedulable_request(OperationType past_operation) {
        std::lock_guard<std::mutex> lock(mtx);
#ifdef DEBUG_MP3_REQUEST_QUEUES
        std::cout << "NAME_NODE_REQUEST_QUESES: " << "Trying to find a new request to schedule. #reads: " << num_reads_running << " #writes:" << num_writes_running << std::endl;
#endif
        if (num_reads_running == 2 || num_writes_running == 1) {
            std::cout << "NAME_NODE_REQUEST_QUESES: " << "WARNING: Tried to schedule a request when peak requests were already running" << std::endl;
            return -1; // we are already running requests at peak concurrency
        }
        assert(num_reads_running <= 1 && num_writes_running == 0);
        if (not replica_repair_queue.empty()) {
            return schedule_replica_repair_request();
        }
        if (not read_repair_queue.empty()) {
            if (num_reads_running != 0) {
                std::cout << "NAME_NODE_REQUEST_QUESES: " << "Read repair is pending, but there is a read running right now. So, not scheduling anything" << std::endl;
                return -1; // the next request we run MUST be the read repair, but we can't run read repair in presence of other reads; so let the current reads finish
            }
            assert(num_reads_running == 0 && num_writes_running == 0);
            return schedule_read_repair_request();
        }
        if (read_request_queue.empty() && write_request_queue.empty()) {
            return -1; // we have nothing to run
        }
        // handle starvation edge cases
        if (num_consecutive_reads >= 4 && !write_request_queue.empty()) {
            return schedule_write_request();
        }
        if (num_consecutive_writes >= 4 && !read_request_queue.empty()) {
            return schedule_read_request();
        }
        // If a queue is empty, we can only pick requests from the other non-empty queue
        if (read_request_queue.empty()) {
            return schedule_write_request();
        }
        if (write_request_queue.empty()) {
            return schedule_read_request();
        }
        // We can schedule either a read or a write at this point
#ifdef TEMPORAL_ORDER
        const Request& read_request = read_request_queue.front();
        const Request& write_request = write_request_queue.front();
        if (read_request.get_metadata()->register_timestamp < write_request.get_metadata()->register_timestamp) {
            return schedule_read_request();
        } else if (num_reads_running == 0) {
            return schedule_write_request();
        } else { // there is a read currently running so we cannot schedule our write operation
            return -1;
        }
#else
        // if we aleady have a read running, schedule another read. 
        if (num_reads_running == 1) {
            if (!read_request_queue.empty()) {
                return schedule_read_request();
            }
            // we still have a read running, so we can't schedule a write. But we have no reads to run, so we cannot run anything right now
            return -1;
        }
        assert(num_reads_running == 0);
        // There is no read currently running, so we will pick the next request based on the past one.
        // If past operation was read, schedule another read or schedule a write
        if (past_operation == OperationType::READ) {
            return schedule_read_request();
        }
        return schedule_write_request();
#endif
    }
};

class FileInformation {
    
private:
    std::string file_name; 	
    uint32_t update_counter;            // Most recent version id, require consistency in all replica
    uint32_t last_committed_version;    // most rescent version id that we know has gone to complition on all replicas
    std::unordered_set<uint32_t> replica_ids; //save the machine ids having replicas
    std::mutex mtx;                // Mutex to guard concurrent access to this class
    std::mutex ack_mutex;   //ack in new thread, mutex to handle shared variable
    int count_ACK;  //To record the number of ACK from replica machines
    bool tombstone; // if set, indicates that the file has been deleted
    BlockManager* block_manager;
    RequestQueues request_manager;
    
public:

    void lock();
    void unlock();
    
    FileInformation& operator=(FileInformation&& other);
    FileInformation(FileInformation&& other);
    void move_helper(FileInformation& other);
    FileInformation(const std::string& new_file_name, bool generate_replicas, BlockManager* block_manager);
    FileInformation& operator=(const FileInformation& other);

    std::string get_file_name() const;
    uint32_t _get_update_counter() const;
    void set_update_counter(uint32_t new_update_counter);
    uint32_t _get_last_committed_version() const;
    void set_last_committed_version(uint32_t new_last_committed_version);
    const std::unordered_set<uint32_t>& get_replica_ids() const;
    void _set_last_committed_version(uint32_t new_last_committed_version);
    void _set_update_counter(uint32_t new_update_counter);
    bool is_tombstone();

    void _add_replica(uint32_t replica_id);
    void _delete_replica(uint32_t replica_id);
    bool _contains_replica(uint32_t replica_id) const;
    void _generate_replica_machineIDs(bool lock_membership = true);

    void _set_request_fields(uint32_t client_fd, Request& req);
    void set_request_fields(uint32_t client_fd, Request& req);
    void manage_request_queue(Request& req);
    Request pop_request_from_queue();
    
    void handle_response(std::shared_ptr<Request> request, int data_node_server_socket, uint32_t datanode_id);
    void handle_ls_request(Request& current_request);
    void handle_read_repair(std::shared_ptr<Request>& request);

    RequestQueues& get_request_manager();

    std::unordered_set<int> get_destination_sockets(const std::unordered_map<int, int>& replica_sockets) const;
    void set_replica_sockets_to_invalid(std::unordered_map<int, int>& replica_sockets) const;
    void start_replica_threads(std::shared_ptr<Request> current_request, std::unordered_map<int, int> replica_sockets);
    void send_request_data_from_socket_to_others(std::shared_ptr<Request> request, std::unordered_map<int, int> replica_sockets);
    void send_request_data_from_file_to_others(std::shared_ptr<Request> request, std::unordered_map<int, int> replica_sockets);

    void schedule_to_datanode(OperationType past_operation);
    void send_request_to_datanode(uint32_t current_request_id);
};
