#include <arpa/inet.h>

#include <cassert>
#include <cstring>
#include <chrono>
#include <mutex>
#include <iostream>
#include <fstream>
#include <functional>
#include <netdb.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <unordered_set>
#include <netdb.h>
#include <arpa/inet.h>
#include <fstream>

#include "utils.hpp"
#include "request.hpp"
#include "response.hpp"

#include "../mp1/utils.hpp"
#include "../mp2/utils.hpp"

bool compare_data(const char* d1, const char* d2, uint32_t len) {
	for (uint32_t i = 0; i < len; ++i) {
		if (d1[i] != d2[i]) {
			return false;
		}
	}
	return true;
}

bool send_request_over_socket(int socket, const Request& request, std::string print_statement_origin, request_data_serializer data_serializer = nullptr) {
	uint32_t payload_size = request.struct_serialized_size();
	uint32_t total_message_size = payload_size + sizeof(uint32_t) + sizeof(uint32_t); // +sizeof(uint32_t) to send payload_size and the footer

	uint32_t network_message_size = htonl(payload_size);
	try {
		send(socket, (char*)&network_message_size, sizeof(uint32_t), MSG_NOSIGNAL);
		if (errno == EPIPE) {
			return false;
		}
	} catch (...) {
		std::cout << print_statement_origin << "Socket closed while trying to serialize request" << std::endl;
		return false;
	}

	try {
		uint32_t sent_message_size = request.serialize(socket, data_serializer);
		#ifdef DEBUG_MP3
				assert(sent_message_size == payload_size);
		#endif
	} catch (...) {
		std::cout << print_statement_origin << "Socket closed while trying to serialize request" << std::endl;
		return false;
	}
	if (errno == EPIPE) {
		return false;
	}

#ifdef DEBUG_MP3
	std::cout << print_statement_origin << "Sending request over socket" << std::endl;
#endif

	return true;
}

bool get_request_over_socket(int socket, Request& request, std::string print_statement_origin) {
	uint32_t payload_size = 0;
	if (read_from_socket(socket, (char*)&payload_size, sizeof(uint32_t)) != sizeof(uint32_t)) {
		std::cout << print_statement_origin << "Client did not include packet size in the message" << std::endl;
		return false;
	}
	payload_size = ntohl(payload_size);

	if (request.deserialize_self(socket, print_statement_origin) < 0) {
		std::cout << print_statement_origin << "Client did not send the correct request header" << std::endl;
		return false;
	}
	return true;
}

void close_all_sockets(const std::unordered_set<int>& destination_sockets) {
	for (uint32_t destination_socket : destination_sockets) {
		close(destination_socket);
	}
}

bool send_request_data_from_socket(std::unordered_set<int> destination_sockets, Request& request, std::string print_statement_origin, int32_t max_failed_sockets = -1) {
	if (max_failed_sockets < 0) {
		max_failed_sockets = destination_sockets.size() + 1;
	}
#ifdef DEBUG_MP3
	std::cout << print_statement_origin << std::this_thread::get_id() << " Sending data bytes to request on file " << request.get_file_name() << std::endl;
#endif
	char* buffer = nullptr;
	int32_t buffer_size = 0;
	uint32_t total_size = 0;
	std::unordered_set<int> failed_sockets;
	uint32_t i = 0;
	while ((buffer_size = request.read_data_block(buffer, print_statement_origin)) > 0) {
#ifdef DEBUG_MP3
		if (i % 128 == 0) {
			std::cout << print_statement_origin << std::this_thread::get_id() << " Sent " << total_size << " bytes from request on file " << request.get_file_name() << std::endl;
		}
		++i;
#endif
		for (uint32_t destination_socket : destination_sockets) {
			try {
				if (failed_sockets.find(destination_socket) == failed_sockets.end()) {
					send(destination_socket, buffer, buffer_size, MSG_NOSIGNAL);
				}
			} catch (...) {}
			if (errno == EPIPE) { // the destination closed the socket (likely because it crashed)
#ifdef DEBUG_MP3
			if (failed_sockets.find(destination_socket) == failed_sockets.end()) {
				std::cout << print_statement_origin << "file " << request.get_file_name() << " - failed to send file as socket " << destination_socket << " raied EPIPE" << std::endl; 
			}
#endif
				failed_sockets.insert(destination_socket);
				if (failed_sockets.size() >= max_failed_sockets) {
					return false;
				}
			}
			errno = 0;
		}
		total_size += buffer_size;
		delete[] buffer;
		buffer = nullptr;
		buffer_size = 0;
	}
	
	uint32_t network_footer = htonl(FOOTER);
	for (uint32_t destination_socket : destination_sockets) {
		try {
			if (failed_sockets.find(destination_socket) == failed_sockets.end()) {
				send(destination_socket, (char*)&network_footer, sizeof(uint32_t), MSG_NOSIGNAL);
			}
			if (errno == EPIPE) { // the destination closed the socket (likely because it crashed)
#ifdef DEBUG_MP3
				if (failed_sockets.find(destination_socket) == failed_sockets.end()) {
					std::cout << print_statement_origin << "file " << request.get_file_name() << " - failed to send file as socket " << destination_socket << " raied EPIPE" << std::endl; 
				}
#endif
				failed_sockets.insert(destination_socket);
				if (failed_sockets.size() >= max_failed_sockets) {
					return false;
				}
			}
			errno = 0;
			shutdown(destination_socket, SHUT_WR);
		} catch (...) {}
	}
#ifdef DEBUG_MP3
	std::cout << print_statement_origin << std::this_thread::get_id() << " Sent " << total_size << " bytes from request on file " << request.get_file_name() << std::endl;
#endif
	return buffer_size == 0;
}

bool send_request_data_from_file(std::unordered_set<int> destination_sockets, Request& request, std::string print_statement_origin, int32_t max_failed_sockets = -1) {
	if (max_failed_sockets < 0) {
		max_failed_sockets = destination_sockets.size() + 1;
	}
	assert(request.get_request_type() == OperationType::WRITE);
	WriteMetadata* metedata = reinterpret_cast<WriteMetadata*>(request.get_metadata());

#ifdef DEBUG_MP3
	std::cout << print_statement_origin << std::this_thread::get_id() << " Starting to send bytes of local file " << metedata->get_local_file_name() << std::endl;
#endif
	std::fstream file;
	file.open(metedata->get_local_file_name(), std::ofstream::in);
	if (!file) {
		return false;
	}

	file.seekp(0, std::ios::end);
	uint32_t file_size = file.tellp();
	file.seekg(0, std::ios::beg);

	char* buffer = new char[REQUEST_DATA_BUFFER_SIZE];
	std::unordered_set<int> failed_sockets;
	uint32_t j = 0;
	for (uint32_t i = 0; i < file_size; i += REQUEST_DATA_BUFFER_SIZE) {
		uint32_t buffer_size = MIN(REQUEST_DATA_BUFFER_SIZE, file_size - i);
		file.read(buffer, buffer_size);
#ifdef DEBUG_MP3
		if (j % 128 == 0) {
			std::cout << print_statement_origin << std::this_thread::get_id() << " Sent " << i << " of " << file_size << " bytes of local file " << metedata->get_local_file_name() << std::endl;
		}
		++j;
#endif
		for (uint32_t destination_socket : destination_sockets) {
			try {
				if (failed_sockets.find(destination_socket) == failed_sockets.end()) {
#ifdef DEBUG_MP3
					std::cout << print_statement_origin << "Sending bytes of local file " << metedata->get_local_file_name() << " to socket " << destination_socket << std::endl;
#endif
					send(destination_socket, buffer, buffer_size, MSG_NOSIGNAL);
				}
			} catch (...) {}
			if (errno == EPIPE) { // the destination closed the socket (likely because it crashed)
#ifdef DEBUG_MP3
				if (failed_sockets.find(destination_socket) == failed_sockets.end()) {
					std::cout << print_statement_origin << "file " << metedata->get_local_file_name() << " - failed to send file as socket " << destination_socket << " raied EPIPE" << std::endl; 
				}
#endif
				failed_sockets.insert(destination_socket);
				if (failed_sockets.size() >= max_failed_sockets) {
					return false;
				}
			}
			errno = 0;
		}
	}
	delete[] buffer;
	buffer = nullptr;

	file.close();

	uint32_t network_footer = htonl(FOOTER);
#ifdef DEBUG_MP3
	std::cout << print_statement_origin << "Sending footer of local file " << metedata->get_local_file_name() << std::endl;
#endif
	for (uint32_t destination_socket : destination_sockets) {
		try {
			if (failed_sockets.find(destination_socket) == failed_sockets.end()) {
#ifdef DEBUG_MP3
				std::cout << print_statement_origin << "Sending bytes of buffer to socket " << destination_socket << std::endl;
#endif
				send(destination_socket, (char*)&network_footer, sizeof(uint32_t), MSG_NOSIGNAL);
			}
			if (errno == EPIPE) { // the destination closed the socket (likely because it crashed)
#ifdef DEBUG_MP3
				if (failed_sockets.find(destination_socket) == failed_sockets.end()) {
					std::cout << print_statement_origin << "file " << metedata->get_local_file_name() << " - failed to send file as socket " << destination_socket << " raied EPIPE" << std::endl; 
				}
#endif
				failed_sockets.insert(destination_socket);
				if (failed_sockets.size() >= max_failed_sockets) {
					return false;
				}
			}
			errno = 0;
			shutdown(destination_socket, SHUT_WR);
		} catch (...) {}
	}

#ifdef DEBUG_MP3
        std::cout << print_statement_origin << std::this_thread::get_id() << " Sent " << file_size << " bytes of local file " << metedata->get_local_file_name() << std::endl;
#endif
        return true;
}

bool send_response_over_socket(int socket, const Response& response, std::string print_statement_origin) {
#ifdef DEBUG_MP3
        std::cout << print_statement_origin << "Sending basic response (type Response)" << std::endl;
#endif
        uint32_t payload_size = response.struct_serialized_size();
        uint32_t total_message_size = payload_size + sizeof(uint32_t);
        char* buffer = new char[total_message_size];
	char* buffer_start = buffer;

        uint32_t network_message_size = htonl(payload_size);
        memcpy(buffer, (char*)&network_message_size, sizeof(uint32_t));
        buffer += sizeof(uint32_t);

        uint32_t sent_message_size = response.serialize(buffer);

#ifdef DEBUG_MP3
        assert(sent_message_size == payload_size);
#endif

        send(socket, buffer_start, total_message_size, 0);

        delete[] buffer_start;
        buffer = nullptr;
        return true;
}

bool send_response_over_socket(int socket, const WriteSuccessReadResponse& response, std::string print_statement_origin) {
#ifdef DEBUG_MP3
        std::cout << print_statement_origin << "Sending write/delete/successful read response (type WriteSuccessReadResponse)" << std::endl;
#endif
        uint32_t payload_size = response.struct_serialized_size();
        uint32_t total_message_size = payload_size + sizeof(uint32_t);
        char* buffer = new char[total_message_size];
	char* buffer_start = buffer;

        uint32_t network_message_size = htonl(payload_size);
        memcpy(buffer, (char*)&network_message_size, sizeof(uint32_t));
        buffer += sizeof(uint32_t);

        uint32_t sent_message_size = response.serialize(buffer);

#ifdef DEBUG_MP3
        assert(sent_message_size == payload_size);
#endif

        send(socket, buffer_start, total_message_size, 0);

        delete[] buffer_start;
        buffer = nullptr;
        return true;
}

bool send_response_over_socket(int socket, const LSResponse& response, std::string print_statement_origin) {
#ifdef DEBUG_MP3
        std::cout << print_statement_origin << "Sending ls response (type LSResponse)" << std::endl;
#endif
        uint32_t payload_size = response.struct_serialized_size();
        uint32_t total_message_size = payload_size + sizeof(uint32_t);
        char* buffer = new char[total_message_size];
	char* buffer_start = buffer;

        uint32_t network_message_size = htonl(payload_size);
        memcpy(buffer, (char*)&network_message_size, sizeof(uint32_t));
        buffer += sizeof(uint32_t);

        uint32_t sent_message_size = response.serialize(buffer);

#ifdef DEBUG_MP3
        assert(sent_message_size == payload_size);
#endif

        send(socket, buffer_start, total_message_size, 0);

        delete[] buffer_start;
        buffer = nullptr;
        return true;
}

Response* get_response_over_socket(int socket, uint32_t& payload_size) {
	if (read_from_socket(socket, (char*)&payload_size, sizeof(uint32_t)) != sizeof(uint32_t)) {
			std::cout << "UTILS: client did not send payload size in response" << std::endl;
			return nullptr;
	}
	payload_size = ntohl(payload_size);

#ifdef DEBUG_MP3
	std::cout << "UTILS: client sent response of size " << payload_size << " in hex: " << std::hex << payload_size << std::dec << std::endl;
	std::cout << "UTILS: client sent response of in network layout size " << htonl(payload_size) << " in hex: " << std::hex << htonl(payload_size) << std::dec << std::endl;
#endif

	char* buffer = new char[payload_size];
	if (read_from_socket(socket, buffer, payload_size) != payload_size) {
			std::cout << "UTILS: client payload did not match payload size. Expected size " << payload_size << std::endl;
			return nullptr;
	}

	Response basic_response = Response::deserialize(buffer);

	Response* response;
	if (basic_response.get_response_type() == OperationType::WRITE || (
				basic_response.get_response_type() == OperationType::READ && basic_response.get_status_code() >= 0)) {
		WriteSuccessReadResponse* read_write_response = new WriteSuccessReadResponse();
		read_write_response->deserialize_self(buffer);
		response = read_write_response;
	} else if (basic_response.get_response_type() == OperationType::LS) {
		LSResponse* ls_response  = new LSResponse();
		ls_response->deserialize_self(buffer);
		response = ls_response;
	} else {
		response = new Response();
		response->deserialize_self(buffer);
	}

	delete[] buffer;
        buffer = nullptr;
        return response;
}

int get_tcp_socket_with_node(int node_id, int port) {
        std::string machine_ip = get_machine_ip(node_id);
        return setup_tcp_client_socket(machine_ip, port);
}

int create_socket_to_datanode(uint32_t data_node_server_port, uint32_t datanode_id) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));  // Clear the structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data_node_server_port);
    std::string datanode_ip_address = get_machine_ip(datanode_id);
    server_addr.sin_addr.s_addr = inet_addr(datanode_ip_address.c_str()); 

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        perror("connection to the server failed");
        std::cout << "NAME_NODE_FILE_INFORMATION: " << "could not connect with node id " << datanode_id << " on port " << data_node_server_port << std::endl;
        close(sockfd);
        return -1;
    }

    return sockfd;
}