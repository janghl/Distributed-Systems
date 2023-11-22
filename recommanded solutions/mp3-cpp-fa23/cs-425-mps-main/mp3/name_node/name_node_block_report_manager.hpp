#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>
#include "../data_node/data_node_block_report_manager.hpp"
#include "file_information.h"


static void file_not_found(int client_fd, std::string filename, OperationType operation) {
	if (operation != OperationType::LS) {
		Response response(self_machine_number, operation, FILE_NOT_FOUND_ERROR_CODE, filename);
		send_response_over_socket(client_fd, response, "NAME_NODE_FILE_INFORMATION: ");
	} else {
		LSResponse response(self_machine_number, OperationType::LS, 0, filename, {});
		send_response_over_socket(client_fd, response, "NAME_NODE_FILE_INFORMATION: ");
	}
}

//Update its metadata regarding where each block of data resides.
//Check for any missing blocks.
class BlockManager {
private:  
	//key = file name, value = File Info
	std::unordered_map<std::string, FileInformation> global_files; // global view of all files
	bool name_node_inited;
	std::mutex mtx; //global lock

	void _register_file(const std::string& filename) {
		if (file_exists(filename)) {
				return;
		}
		global_files.try_emplace(filename, filename, true, this);
	}

	FileInformation& _get_file_info(const std::string& filename) {
		if(file_exists(filename)) {
				return global_files.at(filename);
		}
		throw std::runtime_error("File not found");
	}

	// Process a block report received from a DataNode
	// We call this when we detect we are the newly elected leader
	void _process_block_report(uint32_t dataNodeID, const BlockReport* report) {
		for (auto& file_meta : report->get_files_metadata()) {
			const std::string& file_name = file_meta.first;
			const uint32_t reported_update_counter = file_meta.second;

			// Check if file already exists in global metadata
			if (global_files.find(file_name) == global_files.end()) {
				FileInformation info(file_name, false, this);
				info._set_update_counter(reported_update_counter);
				info._set_last_committed_version(reported_update_counter);
				info._add_replica(dataNodeID);
				global_files.emplace(file_name, std::move(info));
			} else {
				// Existing file, check update counters and add/ensure this DataNode is listed as having a replica
				FileInformation& info = global_files.at(file_name);
				info.lock();
				if (info._get_last_committed_version() <= reported_update_counter) {
					info._set_last_committed_version(reported_update_counter);
				}
				if (info._get_update_counter() <= reported_update_counter) {
					info._set_update_counter(reported_update_counter);
				}
				info._add_replica(dataNodeID);
				info.unlock();
			}
		}
	}

public:
	BlockManager() : name_node_inited(false) {}

	void register_file(const std::string& filename) {
		std::lock_guard<std::mutex> lock(mtx);
		_register_file(filename);
	}

	std::unordered_set<uint32_t> get_DataNodes(const std::string& filename) {
		std::lock_guard<std::mutex> lock(mtx);
		if(file_exists(filename)){
			return global_files.at(filename).get_replica_ids();
		}
		return {};  
	}

	bool inited() {
		return this->name_node_inited;
	}

	FileInformation& get_file_info(const std::string& filename) {
		std::lock_guard<std::mutex> lock(mtx);
		return _get_file_info(filename);
	}

	bool file_exists(std:: string filename){
		return global_files.find(filename) != global_files.end();
	}

	void delete_file(std::string filename) {
		std::lock_guard<std::mutex> lock(mtx);
		global_files.erase(filename);
	}

	void init_self() {
		std::lock_guard<std::mutex> lock(mtx);

		MemberInformation& member = membership_list_manager.self_get_status();
		member.lock();
		BlockReport* report = reinterpret_cast<BlockReport*>(member.get_extra_information());
		_process_block_report(member.get_machine_id(), report);
		member.unlock();

		std::vector<MemberInformation>& members = membership_list_manager.get_membership_list();
		for (MemberInformation& member : members) {
			member.lock();
			if (!member.has_failed()) {
				BlockReport* report = reinterpret_cast<BlockReport*>(member.get_extra_information());
				_process_block_report(member.get_machine_id(), report);
			}
			member.unlock();
		}

		for (auto it = global_files.begin(); it != global_files.end(); ++it) {
			if (NUM_REPLICAS <= it->second.get_replica_ids().size()) {
				continue;
			}
			it->second.lock();
			it->second._generate_replica_machineIDs();
			Request request = Request(OperationType::READ, it->first, nullptr);
			it->second._set_request_fields(-1, request);
			(reinterpret_cast<ReadMetadata*>(request.get_metadata()))->replica_repair = true;
			RequestQueues& request_manager = it->second.get_request_manager();

			request_manager.register_replica_repair_request(request);
#ifdef DEBUG_MP3_NAME_NODE_BLOCK_REPORT
			std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: " << "registering replica repair on file " << it->first << std::endl;
			assert(request_manager.can_try_scheduling_requests());
			std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: " << "currently running request before exctuing replica repair " << 
							operation_to_string(request_manager.get_current_running_request_type()) << std::endl;
#endif
			it->second.unlock();
			it->second.schedule_to_datanode(request_manager.get_current_running_request_type());
		}
		name_node_inited = true;
	}

	void _handle_data_node_failure_impl(uint32_t member_id) {
		for (auto it = global_files.begin(); it != global_files.end(); ++it) {
			FileInformation& info = it->second;
			info.lock();
			if (!info._contains_replica(member_id)) {
#ifdef DEBUG_MP3_NAME_NODE_BLOCK_REPORT
				std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: file " << it->first << " does not have replica id " << member_id << std::endl;
#endif
				info.unlock();
				continue;
			}

			info._delete_replica(member_id);
			info._generate_replica_machineIDs(false);

			Request request = Request(OperationType::READ, it->first, nullptr);
			info._set_request_fields(-1, request);
			(reinterpret_cast<ReadMetadata*>(request.get_metadata()))->replica_repair = true;

			RequestQueues& request_manager = info.get_request_manager();

			request_manager.register_replica_repair_request(request);
#ifdef DEBUG_MP3_NAME_NODE_BLOCK_REPORT
			std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: " << "registering replica repair on file " << it->first << std::endl;
			std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: " << "currently running request before exctuing replica repair " << 
							operation_to_string(request_manager.get_current_running_request_type()) << std::endl;
			std::string replicas;
			for (auto i : info.get_replica_ids()) {
				replicas += std::to_string(i) + " ";
			}
			std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: file " << it->first << " replica ids: " <<  replicas << std::endl;
#endif
			info.unlock();
			info.schedule_to_datanode(request_manager.get_current_running_request_type());
		}
	}

	void handle_data_node_failure(MemberInformation& member) {
		std::lock_guard<std::mutex> lock(mtx);

#ifdef DEBUG_MP3_NAME_NODE_BLOCK_REPORT
		std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: data node failure handler called for member: " << member.get_machine_id() << std::endl;
#endif
		if (name_node_id.load() != self_machine_number || !name_node_inited) {
#ifdef DEBUG_MP3_NAME_NODE_BLOCK_REPORT
			std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: we are not the name node or name node is not inited; not handling data node failure" << std::endl;
#endif
			return;
		}
		
		std::thread send_data_thread = std::thread(&BlockManager::_handle_data_node_failure_impl, this, member.get_machine_id());
        send_data_thread.detach(); // detach so the thread can run independently of the main thread
	}

	int distribute_request(int client_fd, Request& req) {
		std::lock_guard<std::mutex> lock(mtx);
#ifdef DEBUG_MP3_NAME_NODE_BLOCK_REPORT
		std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: " << "got request on file " << req.get_file_name() << std::endl;
#endif
		if (!file_exists(req.get_file_name())) {
#ifdef DEBUG_MP3_NAME_NODE_BLOCK_REPORT
			std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: " << "file " << req.get_file_name() << " does not exist" << std::endl;
#endif
			// request is for a while that doesn't currently exist; return 'not found' error for any request that is not
			// WRITE
			if (req.get_request_type() != OperationType::WRITE) {
				file_not_found(client_fd, req.get_file_name(), req.get_request_type());
				close(client_fd);
				return -1;
			}
			_register_file(req.get_file_name());
		}
		FileInformation& file = _get_file_info(req.get_file_name());
#ifdef DEBUG_MP3_NAME_NODE_BLOCK_REPORT
		std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: " << "sending metdata for request on file " << req.get_file_name() << std::endl;
#endif
		file.set_request_fields(client_fd, req);
#ifdef DEBUG_MP3_NAME_NODE_BLOCK_REPORT
		std::cout << "NAME_NODE_BLOCK_REPORT_MANAGER: " << "queueing request on file " << req.get_file_name() << std::endl;
#endif
		file.manage_request_queue(req);
		return 0;
	}
};
