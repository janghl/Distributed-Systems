#include <vector>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>

#include "utils.hpp"
#include "data_node_server.hpp"
#include "data_node_file_manager.hpp"
#include "operation_type.hpp"
#include "request.hpp"
#include "response.hpp"

#include "../mp2/utils.hpp"

BlockReport* block_report_global;

void launch_data_node_server(int port, pthread_t& thread, BlockReport* block_report_ptr) {
	block_report_global = block_report_ptr;
	int server_socket = setup_tcp_server_socket(port);
	pthread_create(&thread, NULL, run_data_node_server, reinterpret_cast<void*>(server_socket));
}

void handle_data_node_write(int client_fd, Request& client_request, BlockReport* block_report) {
#ifdef DEBUG_MP3_DATA_NODE_SERVER
	std::cout << "DATA_NODE_SERVER: " << "Writing to file " << client_request.get_file_name() << std::endl;
	std::cout << "DATA_NODE_SERVER: " << "New counter of file should be " << client_request.get_update_counter() << std::endl;
#endif
	assert(client_fd == client_request.get_socket());
	int status = file_write(client_request.get_file_name(), client_request, client_request.get_data_length(), 
								client_request.get_update_counter(), block_report);
#ifdef DEBUG_MP3_DATA_NODE_SERVER
	std::cout << "DATA_NODE_SERVER: " << "Write completed with status " << status << std::endl;
#endif
	WriteSuccessReadResponse response(self_machine_number, OperationType::WRITE, status, client_request.get_file_name(), 
										client_request.get_update_counter());
#ifdef DEBUG_MP3_DATA_NODE_SERVER
	std::cout << "DATA_NODE_SERVER: " << "Sending response to client" << std::endl;
#endif
	send_response_over_socket(client_fd, response, "DATA_NODE_SERVER: ");
}

void handle_data_node_delete(int client_fd, Request& client_request, BlockReport* block_report) {
#ifdef DEBUG_MP3_DATA_NODE_SERVER
	std::cout << "DATA_NODE_SERVER: " << "Deleting file " << client_request.get_file_name() << std::endl;
#endif
	int status = file_delete(client_request.get_file_name(), block_report);
#ifdef DEBUG_MP3_DATA_NODE_SERVER
	std::cout << "DATA_NODE_SERVER: " << "Delete completed with status " << status << std::endl;
#endif
	Response response(self_machine_number, OperationType::DELETE, status, client_request.get_file_name());
#ifdef DEBUG_MP3_DATA_NODE_SERVER
	std::cout << "DATA_NODE_SERVER: " << "Sending response to client" << std::endl;
#endif
	send_response_over_socket(client_fd, response, "DATA_NODE_SERVER: ");
}

void handle_data_node_read(int client_fd, Request& client_request, BlockReport* block_report) {
	char* file_data_ptr;
	uint32_t file_data_len;

	uint32_t file_update_counter = 0;
	int status = file_read(client_request.get_file_name(), file_data_ptr, file_data_len, file_update_counter, block_report);
#ifdef DEBUG_MP3_DATA_NODE_SERVER
	std::cout << "DATA_NODE_SERVER: " << "Read from file " << client_request.get_file_name() << " with counter " << file_update_counter << 
		"; read status " << status << std::endl;
#endif
	if (status < 0) {
#ifdef DEBUG_MP3_DATA_NODE_SERVER
		std::cout << "DATA_NODE_SERVER: " << "Sending read error response" << std::endl;
#endif
		Response response(self_machine_number, OperationType::READ, status, client_request.get_file_name());
		send_response_over_socket(client_fd, response, "DATA_NODE_SERVER: ");
	} else {
#ifdef DEBUG_MP3_DATA_NODE_SERVER
		std::cout << "DATA_NODE_SERVER: " << "Sending read success response with update counter " << file_update_counter << std::endl;
#endif
		WriteSuccessReadResponse response(self_machine_number, OperationType::READ, status, client_request.get_file_name(),
											file_update_counter, file_data_len, file_data_ptr);
		send_response_over_socket(client_fd, response, "DATA_NODE_SERVER: ");
	}
}

void handle_data_node_client(int client_fd) {
	Request client_request;
#ifdef DEBUG_MP3_DATA_NODE_SERVER
	std::cout << "DATA_NODE_SERVER: " << "Getting client request..." << std::endl;
#endif
	if (!get_request_over_socket(client_fd, client_request, "DATA_NODE_SERVER: ")) {
		std::cout << "DATA_NODE_SERVER: " << "Could not parse client request" << std::endl;
		return;
	}
	if (client_request.get_request_type() == OperationType::READ) {
#ifdef DEBUG_MP3_DATA_NODE_SERVER
		std::cout << "DATA_NODE_SERVER: " << "Client sent read request" << std::endl;
#endif
		handle_data_node_read(client_fd, client_request, block_report_global);
	} else if (client_request.get_request_type() == OperationType::WRITE) {
#ifdef DEBUG_MP3_DATA_NODE_SERVER
		std::cout << "DATA_NODE_SERVER: " << "Client sent write request" << std::endl;
#endif
		handle_data_node_write(client_fd, client_request, block_report_global);
	} else if (client_request.get_request_type() == OperationType::DELETE) {
#ifdef DEBUG_MP3_DATA_NODE_SERVER
		std::cout << "DATA_NODE_SERVER: " << "Client sent delete request" << std::endl;
#endif
		handle_data_node_delete(client_fd, client_request, block_report_global);
	} else {
		std::cout << "DATA_NODE_SERVER: " << "Invalid request type: " << static_cast<uint32_t>(client_request.get_request_type()) << std::endl;
	}
	close(client_fd);
}

void* run_data_node_server(void* socket_ptr) {
	log_thread_started("data node server");
	int server_socket = static_cast<int>(reinterpret_cast<intptr_t>(socket_ptr));

	while(not end_session) {
		int new_fd = accept(server_socket, NULL, NULL);
#ifdef DEBUG_MP3_DATA_NODE_SERVER
		std::cout << "DATA_NODE_SERVER: " << "accpeted a new client connection on fd: " << new_fd << std::endl;
#endif
		if (new_fd < 0) {
			perror("Could not accept client");
			break;
		}

		std::thread client_handler = std::thread(&handle_data_node_client, new_fd);
        client_handler.detach(); // detach so the thread can run independently of the main thread
	}

	close(server_socket);
	return NULL;
}
