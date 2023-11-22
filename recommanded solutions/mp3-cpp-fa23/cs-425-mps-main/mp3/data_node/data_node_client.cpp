#include <iostream>
#include <unistd.h>
#include <fstream>
#include <ios>
#include <cassert>
#include <vector>
#include <string>

#include "utils.hpp"
#include "request.hpp"
#include "response.hpp"
#include "operation_type.hpp"
#include "data_node_client.hpp"
#include "data_node_file_manager.hpp"
#include "../mp2/membership_list_manager.hpp"

std::vector<pthread_t> client_threads;
MembershipListManager* membership_list;
uint32_t name_node_server_port_ref;

void launch_client(MembershipListManager* membership_list_ptr, uint32_t name_node_server_port_cpy) {
	membership_list = membership_list_ptr;
	name_node_server_port_ref = name_node_server_port_cpy;
}

uint32_t operation_prefix_length(OperationType operation) {
	if (operation == OperationType::READ || operation == OperationType::WRITE) {
		return 3;
	}
	if (operation == OperationType::DELETE) {
		return 6;
	}
	if (operation == OperationType::LS) {
		return 2;
	}
	return -1;
}

Request* create_request(OperationType operation, std::string command) {
	uint32_t command_index = operation_prefix_length(operation) + 1;
	std::string local_file_name = "";
	std::string sdfs_file_name = "";

	// get the sdfs file name for read/delete/ls and the local file name for write
	if (operation != OperationType::WRITE) {
		int32_t sdfs_file_name_end = command.find(' ', command_index);
		sdfs_file_name = (sdfs_file_name_end == command.npos) ? command.substr(command_index) : command.substr(command_index, sdfs_file_name_end - command_index);
		command_index = sdfs_file_name_end + 1;
	} else {
		uint32_t local_file_name_end = command.find(' ', command_index);
		local_file_name = command.substr(command_index, local_file_name_end - command_index);
		command_index = local_file_name_end + 1;
	}

	// get the sdfs file name for write and the local file name for read
	if (operation == OperationType::READ) {
		local_file_name = command.substr(command_index);
	} else if (operation == OperationType::WRITE) {
		sdfs_file_name = command.substr(command_index);
	}

	if (operation == OperationType::READ) {
		ReadMetadata* read_meta = new ReadMetadata(local_file_name);
        	return new Request(operation, sdfs_file_name, read_meta);
	}
	if (operation == OperationType::WRITE) {
		// Check if local file exists and if yes, record its size

		std::fstream file;
		file.open(local_file_name.c_str(), std::ofstream::in);
		if (!file || not file.good()) {
#ifdef DEBUG_MP3_DATA_NODE_CLIENT
			std::cout << "DATA_NODE_CLIENT: " << "Could not read file " << local_file_name << std::endl;
#endif
			return new Request(OperationType::INVALID, "", nullptr);
		}
	
		file.seekg(0, std::ios::end);
		uint32_t data_length = file.tellg();
		file.seekg(0, std::ios::beg);

#ifdef DEBUG_MP3_DATA_NODE_CLIENT
		std::cout << "DATA_NODE_CLIENT: " << "Creating a write request on local file " << local_file_name << " for sdfs file " << sdfs_file_name << std::endl;
#endif
		// Create request
		WriteMetadata* write_meta = new WriteMetadata(local_file_name);
		return new Request(operation, sdfs_file_name, data_length, 0, write_meta);
	}
	if (operation == OperationType::LS || operation == OperationType::DELETE) {
		return new Request(operation, sdfs_file_name, nullptr);
	}
	return new Request(OperationType::INVALID, "", nullptr);
}

int get_tcp_socket_with_name_node(uint32_t name_node_server_port) {
	uint32_t possible_name_node_id = name_node_id.load();
	int socket = socket = get_tcp_socket_with_node(possible_name_node_id, name_node_server_port);

	while (socket < 0) { // currently recorded name node has died
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // give time for a new node name to come up
		possible_name_node_id = membership_list->get_lowest_node_id();
		socket = get_tcp_socket_with_node(possible_name_node_id, name_node_server_port);
	};
	name_node_id.store(possible_name_node_id);
	return socket;
}

Response* try_sending_request(Request* request, int& socket, uint32_t name_node_server_port) {
	socket = get_tcp_socket_with_name_node(name_node_server_port);
#ifdef DEBUG_MP3_DATA_NODE_CLIENT
	std::cout << "DATA_NODE_CLIENT: " << "sending request to name node (node id: " << name_node_id << ")" << std::endl;
#endif
	send_request_over_socket(socket, *request, "DATA_NODE_CLIENT: ", send_request_data_from_file);
#ifdef DEBUG_MP3_DATA_NODE_CLIENT
	std::cout << "DATA_NODE_CLIENT: " << "waiting to get name nodes response..." << std::endl;
#endif
	uint32_t payload_size;
	Response* response = get_response_over_socket(socket, payload_size);
#ifdef DEBUG_MP3_DATA_NODE_CLIENT
	std::cout << "DATA_NODE_CLIENT: " << "got response from name node" << std::endl;
#endif
	return response;
}

void* manage_client_request(void* request_ptr, uint32_t name_node_server_port) {
	Request* request = reinterpret_cast<Request*>(request_ptr);
#ifdef DEBUG_MP3_DATA_NODE_CLIENT
	std::cout << "DATA_NODE_CLIENT: " << "starting to manage request" << std::endl;
#endif

	int socket;
	Response* response = try_sending_request(request, socket, name_node_server_port);
	while (response == nullptr) { // name node died and the new name node hasn't started accepting new requests yet
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // give time for a new node name to come up
		response = try_sending_request(request, socket, name_node_server_port);
	}

	if (response->get_response_type() == OperationType::READ && response->get_status_code() >= 0) {
#ifdef DEBUG_MP3_DATA_NODE_CLIENT
		std::cout << "DATA_NODE_CLIENT: " << "read request successfully; writing data to local file" << std::endl;
#endif
		WriteSuccessReadResponse* read_response = reinterpret_cast<WriteSuccessReadResponse*>(response);

		std::ofstream file(reinterpret_cast<ReadMetadata*>(request->get_metadata())->get_local_file_name(), 
				std::ofstream::out | std::ofstream::trunc);
		if (!file) {
			std::cout << "DATA_NODE_CLIENT: " << "Could not open file for writing" << std::endl;
			std::perror("ofsteam open failed");
			return nullptr;
		}
		file.write(read_response->get_data(), read_response->get_data_length());
		file.close();
	}

	if (response->get_status_code() < 0) {
		std::cout << operation_to_string(response->get_response_type()) << " on file " << response->get_file_name() 
			<< " failed with status code " << response->get_status_code() << std::endl;
	} else if (response->get_response_type() != OperationType::LS) {
		std::cout << operation_to_string(response->get_response_type()) << " on file " << response->get_file_name() 
			<< " succedded with status code " << response->get_status_code() << std::endl;
	} 
	
	if (response->get_status_code() >= 0 && response->get_response_type() == OperationType::WRITE) {
		delete response;
		uint32_t payload_size;
		response = get_response_over_socket(socket, payload_size); // get LS response which is piggybacked with the write response
	}
	if (response->get_status_code() >= 0 && response->get_response_type() == OperationType::LS) {
		LSResponse* ls_response = reinterpret_cast<LSResponse*>(response);
		std::cout << operation_to_string(response->get_response_type()) << " on file " << response->get_file_name() << std::endl;
		for (const uint32_t node_id : ls_response->get_replicas()) {
				std::cout  << "\t" << node_id << std::endl; 
		}
	}

	close(socket);
	delete request;
	delete response;
	return nullptr;
}

void handle_client_command(OperationType operation, std::string command) {
	handle_client_command(operation, command, name_node_server_port_ref);
}

void handle_client_command(OperationType operation, std::string command, uint32_t name_node_server_port) {
	Request* request = create_request(operation, command);
#ifdef DEBUG_MP3_DATA_NODE_CLIENT
	std::cout << "DATA_NODE_CLIENT: " << "created request class for command `" << command << "`" << std::endl;
#endif
	if (request->get_request_type() == OperationType::INVALID) {
		std::cout << "DATA_NODE_CLIENT: " << "Cannot execute request; type invalid" << std::endl;
		return;
	}

	// client_threads.emplace_back();
#ifdef DEBUG_MP3_DATA_NODE_CLIENT
	std::cout << "DATA_NODE_CLIENT: " << "launching thread to manage the request" << std::endl;
#endif
	std::thread client_handler = std::thread(&manage_client_request, request, name_node_server_port);
	client_handler.detach(); // detach so the thread can run independently of the main thread
	// pthread_create(&client_threads.back(), NULL, manage_client_request, request);

	return;
}
