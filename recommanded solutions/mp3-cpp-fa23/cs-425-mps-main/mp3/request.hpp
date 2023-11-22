#pragma once 
#include <arpa/inet.h>

#include <cassert>
#include <functional>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <queue>
#include <thread>

#include "operation_type.hpp"
#include "utils.hpp"
#include "request_metadata.hpp"

#include "../mp1/utils.hpp"
#include "../mp2/utils.hpp"

class Request {
private:
	OperationType request_type;
	std::string file_name;		// sdfsfilename - for all request types
	uint32_t data_length;		// filedata length - for WRITE requests
	uint32_t update_counter;	// fileupdatecounter - for WRITE requests

	// Fields not serialized
	Metadata* metadata;
	int socket;
	uint32_t deserialized_data_length;

public:
	Request() : request_type(OperationType::INVALID), file_name(""), data_length(0), update_counter(0), metadata(nullptr), socket(0), deserialized_data_length(0) {}

	// Constructor for read and delete requests
	Request(OperationType type, const std::string& fname, Metadata* metadata = nullptr)
		: request_type(type), file_name(fname), data_length(0), update_counter(0), metadata(metadata), socket(0), deserialized_data_length(0) {
			if (type == OperationType::WRITE) {
				throw std::invalid_argument("For WRITE requests, data is required.");
			}
		}

	// Constructor for write requests
	Request(OperationType type, const std::string& fname, uint32_t data_length, uint32_t update_counter, 
			Metadata* meta = nullptr) : 
		request_type(type), file_name(fname), data_length(data_length), update_counter(update_counter), 
		metadata(meta), socket(0), deserialized_data_length(0) {
			#ifndef TEST_MODE
			if (type != OperationType::WRITE) {
				throw std::invalid_argument("Data is only for WRITE requests.");
			}
			#endif
		}

	Request(const Request& request) = delete;
	Request& operator=(const Request& request) = delete;

	Request(Request&& request) :
		request_type(request.request_type), file_name(request.file_name), data_length(request.data_length),
		update_counter(request.update_counter), metadata(request.metadata), socket(request.socket), deserialized_data_length(request.deserialized_data_length) {
		request.metadata = nullptr;
	}

	Request& operator=(Request&& request) {
		this->request_type = request.request_type;
		this->file_name = request.file_name;
		this->data_length = request.data_length;
		this->update_counter = request.update_counter;
		this->metadata = request.metadata;
		this->socket = request.socket;
		this->deserialized_data_length = request.deserialized_data_length;

		request.metadata = nullptr;
		return *this;
	}

	~Request() {
		delete metadata;
		metadata = nullptr;
	}

	bool operator==(const Request& other) const {
		return this->request_type == other.request_type &&
			this->file_name == other.file_name &&
			this->data_length == other.data_length &&
			this->update_counter == other.update_counter;
	}

	OperationType get_request_type() const {
		return request_type;
	}

	std::string get_file_name() const {
		return file_name;
	}

	Metadata* get_metadata() const {
		return metadata;
	}

	int get_socket() const {
		return socket;
	}

	uint32_t get_data_length() const {
		if (request_type != OperationType::WRITE) {
                        throw std::logic_error("Data length can be fetched only for WRITE requests.");
                }
		return data_length;
	}

	uint32_t get_update_counter() const {
		if (request_type != OperationType::WRITE) {
                        throw std::logic_error("Update counter can be fetched only for WRITE requests.");
                }
                return update_counter;
	}

	void set_update_counter(uint32_t new_counter) {
		if (request_type != OperationType::WRITE) {
                        throw std::logic_error("Update counter can be fetched only for WRITE requests.");
                }
		this->update_counter = new_counter;
	}

	void set_metadata(Metadata* new_metadata) {
		if (this->metadata != nullptr) {
			delete this->metadata;
		}
		this->metadata = new_metadata;
	}

	bool request_needs_file_name() const {
		return request_type == OperationType::READ || request_type == OperationType::WRITE || request_type == OperationType::DELETE || 
				request_type == OperationType::LS;
	}


	uint32_t struct_header_serialized_size() const {
		return sizeof(uint32_t) + // request_type
			(request_needs_file_name() ? sizeof(uint32_t) + file_name.length() : 0) + 
					// file name length and file name
			(request_type == OperationType::WRITE ? sizeof(uint32_t) + sizeof(uint32_t) : 0); 
					// file update counter, data length, data
	}

	// Returns size of buffer required to serliaze this. this->metadata is not serialized.
	// The structure of the char buffer for each request type is defined in lucid chart:
	// https://lucid.app/lucidchart/c1474ad1-dac7-4fed-80e3-a46ef094b1eb/edit?view_items=-sWEIKGKCENi&invitationId=inv_edc19287-7908-4d96-808e-6e91254cda36
	uint32_t struct_serialized_size() const {
		return sizeof(uint32_t) + // request_type
			(request_needs_file_name() ? sizeof(uint32_t) + file_name.length() : 0) + 
					// file name length and file name
			(request_type == OperationType::WRITE ? sizeof(uint32_t) + sizeof(uint32_t) + data_length : 0); 
					// file update counter, data length, data
	}

	// Serializes fields of this and sends it over the socket. this->metadata is not serialized.
	// The structure of the char buffer for each request type is defined in lucid chart:
	// https://lucid.app/lucidchart/c1474ad1-dac7-4fed-80e3-a46ef094b1eb/edit?view_items=-sWEIKGKCENi&invitationId=inv_edc19287-7908-4d96-808e-6e91254cda36
	uint32_t serialize(int socket, request_data_serializer data_serializer = nullptr) const {
		uint32_t header_size = struct_header_serialized_size();
		uint32_t network_header_size = htonl(header_size);
		send(socket, (char*)&network_header_size, sizeof(uint32_t), 0);

		uint32_t network_request_type = htonl(static_cast<uint32_t>(request_type));
		send(socket, (char*)&network_request_type, sizeof(uint32_t), 0);
	
		if (request_needs_file_name()) {
			uint32_t network_file_name_len = htonl(file_name.length());
			send(socket, (char*)&network_file_name_len, sizeof(uint32_t), 0);

			send(socket, file_name.c_str(), file_name.length(), 0);
		}

		if (request_type == OperationType::WRITE) {
			uint32_t network_update_counter = htonl(update_counter);
			send(socket, (char*)&network_update_counter, sizeof(uint32_t), 0);

			uint32_t network_data_len = htonl(data_length);
			send(socket, (char*)&network_data_len, sizeof(uint32_t), 0);
			std::cout << "REQUEST: sent data length: " << data_length << std::endl;

			if (data_serializer != nullptr) {
				std::cout << "REQUEST: serializing data while serializing request" << std::endl;
				data_serializer({socket}, const_cast<Request&>(*this), "REQUEST: ", -1);
			}
		} else {
			uint32_t network_footer = htonl(FOOTER);
			send(socket, (char*)&network_footer, sizeof(uint32_t), 0);
			shutdown(socket, SHUT_WR);
		}

		return struct_serialized_size();
	}

	// Initialies this using the contents of the buffer. Returns the number of bytes from buffer is used when initializing this.
	int32_t deserialize_self(int socket, std::string print_statement_origin) {
		this->deserialized_data_length = 0;
		this->socket = socket;

		uint32_t header_size;
		if (read_from_socket(socket, (char*)&header_size, sizeof(uint32_t)) != sizeof(uint32_t)) {
			std::cout << print_statement_origin << "Client did not include header size in the message" << std::endl;
			return -1;
        }
		header_size = ntohl(header_size);

		char* buffer_start = new char[header_size];
		char* buffer = buffer_start;
		if (read_from_socket(socket, buffer_start, header_size) != header_size) {
			std::cout << print_statement_origin << "Client did not include header of request in the message" << std::endl;
			return -1;
        }

		uint32_t network_request_type;
		memcpy((char*)&network_request_type, buffer, sizeof(uint32_t));
		this->request_type = static_cast<OperationType>(ntohl(network_request_type));
		buffer += sizeof(uint32_t);

		if (this->request_needs_file_name()) {
			uint32_t file_name_len;
			memcpy((char*)&file_name_len, buffer, sizeof(uint32_t));
			file_name_len = ntohl(file_name_len);
			buffer += sizeof(uint32_t);

			char* file_name_buffer = new char[file_name_len + 1];
			file_name_buffer[file_name_len] = '\0';
			memcpy(file_name_buffer, buffer, file_name_len);
			this->file_name = std::string(file_name_buffer);
			delete[] file_name_buffer;
			buffer += file_name_len;
		}

		if (this->request_type == OperationType::WRITE) {
			uint32_t network_update_counter;
			memcpy((char*)&network_update_counter, buffer, sizeof(uint32_t));
			this->update_counter = ntohl(network_update_counter);
			buffer += sizeof(uint32_t);

			uint32_t data_len;
			memcpy((char*)&data_len, buffer, sizeof(uint32_t));
			this->data_length = ntohl(data_len);
			buffer += sizeof(uint32_t);
		} else {
			uint32_t footer;
			if (read_from_socket(this->socket, (char*)&footer, sizeof(uint32_t)) != sizeof(uint32_t)) {
				std::cout << print_statement_origin << "Client did not include footer of request in the message" << std::endl;
			}
			assert(ntohl(footer) == FOOTER);
		}

		assert(header_size == (buffer - buffer_start));
		return header_size;
	}

	int32_t read_data_block(char*& buffer, std::string print_statement_origin) {
		uint32_t buffer_size = MIN(REQUEST_DATA_BUFFER_SIZE, this->data_length - this->deserialized_data_length);
		if (buffer_size == 0) {
			buffer = nullptr;

			uint32_t footer;
			if (read_from_socket(this->socket, (char*)&footer, sizeof(uint32_t)) != sizeof(uint32_t)) {
				std::cout << print_statement_origin << "Client did not include footer of request in the message" << std::endl;
			}
			assert(ntohl(footer) == FOOTER);

			return 0;
		}
		buffer = new char[buffer_size];
		if (read_from_socket(this->socket, buffer, buffer_size) != buffer_size) {
			std::cout << print_statement_origin << "Client did not include data of request in the message" << std::endl;
			return -1;
        }
		this->deserialized_data_length += buffer_size;
		return buffer_size;
	}

	static Request deserialize(int socket, std::string print_statement_origin) {
		Request request;
		request.deserialize_self(socket, print_statement_origin);
		return request;
	}

};
