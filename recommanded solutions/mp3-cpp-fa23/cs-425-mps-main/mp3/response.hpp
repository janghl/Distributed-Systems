#pragma once 
#include <arpa/inet.h>

#include <cstring>
#include <iostream>
#include <unordered_set>
#include <queue>

#include "operation_type.hpp"
#include "utils.hpp"

class Response {
protected:
	uint32_t node_id;
	OperationType response_type;
	int32_t status_code;
	std::string file_name; // sdfsfilename - for WRITE, READ, and DELETE responses
public:
	Response() : node_id(-1), response_type(OperationType::INVALID), status_code(-1), file_name("") {}

	Response(uint32_t node_id, OperationType response_type, int32_t status_code) :
		node_id(node_id), response_type(response_type), status_code(status_code) {
		if (response_type == OperationType::WRITE || response_type == OperationType::READ || response_type == OperationType::DELETE || response_type == OperationType::LS) {
			throw std::invalid_argument("For READ, DELETE, LS, and WRITE responses, file name is required");
                }
	}

	Response(uint32_t node_id, OperationType response_type, int32_t status_code, std::string file_name) :
		node_id(node_id), response_type(response_type), status_code(status_code), file_name(file_name) {
		if (response_type != OperationType::WRITE && response_type != OperationType::READ && response_type != OperationType::DELETE && response_type != OperationType::LS) {
			throw std::invalid_argument("Filename is only for READ, DELETE, LS, and WRITE responses.");
                }
	}

	bool operator==(const Response& other) const {
		return this->response_type == other.response_type &&
			this->node_id == other.node_id &&
			this->status_code == other.status_code &&
			this->file_name == other.file_name;
	}

	OperationType get_response_type() const {
		return response_type;
	}

	int32_t get_status_code() const {
		return status_code;
	}

	uint32_t get_node_id() const {
		return node_id;
	}

	std::string get_file_name() const {
		return file_name;
	}

	bool response_needs_file_name() const {
                return response_type == OperationType::READ || response_type == OperationType::WRITE || response_type == OperationType::DELETE || 
						response_type == OperationType::LS;
        }

	// Returns size of buffer required to serliaze this. 
	uint32_t struct_serialized_size() const {
		return sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + // node id, response type, and status code
			(response_needs_file_name() ? sizeof(uint32_t) + file_name.length() : 0); // file name length and file name
	}

	// Serializes fields of this into a char buffer allocated on the heap. 
	uint32_t serialize(char*& buffer_start) const {
		uint32_t buffer_size = struct_serialized_size();
		if (buffer_start == nullptr) {
			buffer_start = new char[buffer_size];
		}
		char* buffer = buffer_start;

		uint32_t network_node_id = htonl(node_id);
		memcpy(buffer, (char*)&network_node_id, sizeof(uint32_t));
		buffer += sizeof(uint32_t);

		uint32_t network_response_type = htonl(static_cast<uint32_t>(response_type));
		memcpy(buffer, (char*)&network_response_type, sizeof(uint32_t));
		buffer += sizeof(uint32_t);

		int32_t network_status_code = htonl(status_code);
		memcpy(buffer, (char*)&network_status_code, sizeof(int32_t));
		buffer += sizeof(int32_t);

		if (response_needs_file_name()) {
			uint32_t network_file_name_len = htonl(file_name.length());
                        memcpy(buffer, (char*)&network_file_name_len, sizeof(uint32_t));
                        buffer += sizeof(uint32_t);

                        memcpy(buffer, file_name.c_str(), file_name.length());
                        buffer += file_name.length();
		}
	
		return buffer_size;
	}

	char* deserialize_self(char* buffer) {
                uint32_t network_node_id;
                memcpy((char*)&network_node_id, buffer, sizeof(uint32_t));
                node_id = ntohl(network_node_id);
                buffer += sizeof(uint32_t);

                uint32_t network_response_type;
                memcpy((char*)&network_response_type, buffer, sizeof(uint32_t));
                response_type = static_cast<OperationType>(ntohl(network_response_type));
                buffer += sizeof(uint32_t);

                uint32_t network_status_code;
                memcpy((char*)&network_status_code, buffer, sizeof(uint32_t));
                status_code = ntohl(network_status_code);
                buffer += sizeof(uint32_t);

		if (response_needs_file_name()) {
			uint32_t file_name_len;
                        memcpy((char*)&file_name_len, buffer, sizeof(uint32_t));
                        file_name_len = ntohl(file_name_len);
                        buffer += sizeof(uint32_t);

                        char* file_name_buffer = new char[file_name_len + 1];
                        file_name_buffer[file_name_len] = '\0';
                        memcpy(file_name_buffer, buffer, file_name_len);
                        file_name = std::string(file_name_buffer);
                        delete[] file_name_buffer;
                        buffer += file_name_len;
		}

                return buffer;
        }

	static Response deserialize(char* buffer) {
		Response response;
		response.deserialize_self(buffer);
		return response;
	}

	virtual ~Response() {};
};

class WriteSuccessReadResponse : public Response {
protected:
	uint32_t update_counter;
	uint32_t data_length; 		// for READ
	char* data; 			// for READ

public:
	WriteSuccessReadResponse() : Response(), update_counter(0), data_length(0), data(nullptr) {}

        WriteSuccessReadResponse(uint32_t node_id, OperationType response_type, int32_t status_code, const std::string& fname, 
			uint32_t update_counter, uint32_t data_length, char* data) : 
		Response(node_id, response_type, status_code, fname), update_counter(update_counter), data_length(data_length),
		data(data) { 
			if (response_type != OperationType::READ) {
				throw std::invalid_argument("Data is only for READ response.");
			}
	}

	WriteSuccessReadResponse(uint32_t node_id, OperationType response_type, int32_t status_code, const std::string& fname,
                        uint32_t update_counter) :
                Response(node_id, response_type, status_code, fname), update_counter(update_counter), data_length(0),
                data(nullptr) {
			if (response_type == OperationType::READ) {
				throw std::invalid_argument("For READ, data required");
			}
        }

        WriteSuccessReadResponse(const WriteSuccessReadResponse& response) :
		Response(response), data_length(response.data_length), update_counter(response.update_counter) {
                        if (response.data == nullptr) {
                                data = nullptr;
                        } else {
                                data = new char[data_length];
                                memcpy(data, response.data, data_length);
                        }
        }

	 WriteSuccessReadResponse(WriteSuccessReadResponse&& response) :
		 Response(response), data_length(response.data_length), update_counter(response.update_counter), data(response.data) {
		 }

        ~WriteSuccessReadResponse() override {
                delete[] data;
        }

       	WriteSuccessReadResponse& operator=(const WriteSuccessReadResponse& other) {
		Response::operator=(other);
                this->update_counter = other.update_counter;
                this->data_length = other.data_length;
                if (other.data == nullptr) {
                        this->data = nullptr;
                } else {
                        this->data = new char[this->data_length];
                        memcpy(this->data, other.data, this->data_length);
                }
                return *this;
        }

	uint32_t get_data_length() const {
		return this->data_length;
	}

	const char* get_data() const {
		return this->data;
	}

	uint32_t get_update_counter() const {
		return this->update_counter;
	}

	bool operator==(const WriteSuccessReadResponse& other) const {
                return Response::operator==(other) &&
			this->data_length == other.data_length &&
			this->update_counter == other.update_counter &&
                        ((this->data == nullptr && other.data == nullptr) ||
                         (this->data != nullptr && other.data != nullptr && compare_data(this->data, other.data, this->data_length)));
        }

	// Returns size of buffer required to serliaze this.
        uint32_t struct_serialized_size() const {
		uint32_t response_size = Response::struct_serialized_size();
                return response_size + sizeof(uint32_t) + (response_type == OperationType::READ ? sizeof(uint32_t) + data_length : 0);
        }

	// Serializes fields of this into a char buffer allocated on the heap.
        uint32_t serialize(char*& buffer_start) const {
                uint32_t buffer_size = struct_serialized_size();
                if (buffer_start == nullptr) {
                        buffer_start = new char[buffer_size];
                }
                char* buffer = buffer_start;

		buffer += Response::serialize(buffer);

		uint32_t network_update_counter = htonl(update_counter);
		memcpy(buffer, (char*)&network_update_counter, sizeof(uint32_t));
		buffer += sizeof(uint32_t);

		if (response_type == OperationType::READ) {
			uint32_t network_data_len = htonl(data_length);
			memcpy(buffer, (char*)&network_data_len, sizeof(uint32_t));
			buffer += sizeof(uint32_t);

			memcpy(buffer, data, data_length);
			buffer += data_length;
		}

                return buffer_size;
        }

	char* deserialize_self(char* buffer) {
		buffer = Response::deserialize_self(buffer);

                uint32_t network_update_counter;
                memcpy((char*)&network_update_counter, buffer, sizeof(uint32_t));
                update_counter = ntohl(network_update_counter);
                buffer += sizeof(uint32_t);

		if (response_type == OperationType::READ) {
       			memcpy((char*)&data_length, buffer, sizeof(uint32_t));
			data_length = ntohl(data_length);
			buffer += sizeof(uint32_t);

			this->data = new char[data_length];
			memcpy(data, buffer, data_length);
			buffer += data_length;
		}

                return buffer;
        }

        static WriteSuccessReadResponse deserialize(char* buffer) {
                WriteSuccessReadResponse response;
                response.deserialize_self(buffer);
                return response;
        }
};

class LSResponse : public Response {
protected:
	std::unordered_set<uint32_t> replica_ids;
public:
	LSResponse() : Response() {}

	LSResponse(uint32_t node_id, OperationType response_type, int32_t status_code, const std::string& fname, 
			std::unordered_set<uint32_t> replica_ids) :
		Response(node_id, response_type, status_code, fname), replica_ids(replica_ids) {
	}

	LSResponse(const LSResponse& response) :
		Response(response), replica_ids(response.replica_ids) {}

	 LSResponse(LSResponse&& response) :
		Response(response), replica_ids(std::move(response.replica_ids)) {}

	~LSResponse() {
	}

	LSResponse& operator=(const LSResponse& other) {
		Response::operator=(other);
		this->replica_ids = replica_ids;
		return *this;
	}

	uint32_t get_number_of_replicas() const {
		return this->replica_ids.size();
	}

	const std::unordered_set<uint32_t>& get_replicas() const {
		return this->replica_ids;
	}

	bool operator==(const LSResponse& other) const {
		return Response::operator==(other) && this->replica_ids == other.replica_ids;
	}

	// Returns size of buffer required to serliaze this.
	uint32_t struct_serialized_size() const {
		uint32_t response_size = Response::struct_serialized_size();
		return response_size + sizeof(uint32_t) + (sizeof(uint32_t) * get_number_of_replicas());
	}

	// Serializes fields of this into a char buffer allocated on the heap.
	uint32_t serialize(char*& buffer_start) const {
			uint32_t buffer_size = struct_serialized_size();
			if (buffer_start == nullptr) {
					buffer_start = new char[buffer_size];
			}
			char* buffer = buffer_start;

	buffer += Response::serialize(buffer);

	uint32_t network_num_replicas = htonl(this->replica_ids.size());
	memcpy(buffer, (char*)&network_num_replicas, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	for (const uint32_t replica_id : this->replica_ids) {
		uint32_t network_replica_id = htonl(replica_id);
		memcpy(buffer, (char*)&network_replica_id, sizeof(uint32_t));
		buffer += sizeof(uint32_t);
	}

			return buffer_size;
	}

	char* deserialize_self(char* buffer) {
		buffer = Response::deserialize_self(buffer);

		uint32_t num_replicas;
		memcpy((char*)&num_replicas, buffer, sizeof(uint32_t));
		num_replicas = ntohl(num_replicas);
		buffer += sizeof(uint32_t);

		for (uint32_t i = 0; i < num_replicas; ++i) {
			uint32_t replica_id;
			memcpy((char*)&replica_id, buffer, sizeof(uint32_t));
			replica_id = ntohl(replica_id);
			buffer += sizeof(uint32_t);
			this->replica_ids.insert(replica_id);
		}

		return buffer;
	}

	static LSResponse deserialize(char* buffer) {
			LSResponse response;
			response.deserialize_self(buffer);
			return response;
	}
};


	//for each key: perform a sdfs WRITE with local file name = '[task id]_maple_output_[key]' and sdfs file name = '[task id]_maple_output_[key]'

class MapleSuccessResponse : public Response {
protected:
    std::vector<WriteSuccessReadResponse> write_responses;
    std::vector<std::string> keys;

public:
    MapleSuccessResponse() : Response() {}

    bool operator==(const MapleSuccessResponse& other) const {
        return (this->write_responses == other.write_responses) &&
               (this->keys == other.keys);
    }

    uint32_t struct_serialized_size() const {
        uint32_t response_size = Response::struct_serialized_size();
        uint32_t keys_size = sizeof(uint32_t); // Size for number of keys
        for (const auto& key : keys) {
            keys_size += sizeof(uint32_t) + key.size(); 
        }
        return response_size + keys_size;
    }

    uint32_t serialize(char*& buffer_start) const {
        uint32_t buffer_size = struct_serialized_size();
        if (buffer_start == nullptr) {
            buffer_start = new char[buffer_size];
        }
        char* buffer = buffer_start;

        buffer += Response::serialize(buffer);

        uint32_t network_num_keys = htonl(keys.size());
        memcpy(buffer, &network_num_keys, sizeof(uint32_t));
        buffer += sizeof(uint32_t);

        for (const std::string& key : keys) {
            uint32_t network_key_size = htonl(key.size());
            memcpy(buffer, &network_key_size, sizeof(uint32_t));
            buffer += sizeof(uint32_t);

            memcpy(buffer, key.c_str(), key.size());
            buffer += key.size();
        }

        return buffer_size;
    }

    char* deserialize_self(char* buffer) {
		
    }
};
