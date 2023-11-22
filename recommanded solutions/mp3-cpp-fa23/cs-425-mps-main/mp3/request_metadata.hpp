#pragma once 
#include <cassert>
#include <cstring>
#include <iostream>
#include <chrono>
#include <vector>
#include <queue>
#include <thread>

#include "operation_type.hpp"
#include "utils.hpp"
#include "response.hpp"

// Any metadata that needs to be stored for a request. Metadata is NOT sent to the data node server.
// Inherit from Metadata class to define custom metadata
class Metadata {
protected:
	std::mutex mtx;
public:

	// Variables for NameNode; used to track status of request

	// Variables for general requests
	int client_socket;					// data node client socket the request was recieved over
	std::vector<std::thread> threads; 	// threads executing the request
	uint32_t count_ACKs;				// number of replicas that acked the request
	int32_t request_id;					// unique ID of request (unique per file -- so requests across files can share an id)
	Response* response_to_return; 		// response sent back to data node (set if there is a particular response we want to send)
	uint64_t data_transfer;				// total data transfer (in bytes) required to complete this request
	std::chrono::time_point<std::chrono::system_clock> request_arrival_time;
	std::chrono::time_point<std::chrono::system_clock> request_scheduling_time;
	
	void lock() {
		mtx.lock();
	}

	void unlock() {
		mtx.unlock();
	}

	Metadata() : client_socket(-1), count_ACKs(0), response_to_return(nullptr), data_transfer(0) {}
	Metadata(int socket) : client_socket(socket), count_ACKs(0), response_to_return(nullptr), data_transfer(0) {}

	Metadata& operator=(const Metadata& other) = delete;
	Metadata(const Metadata& other) = delete;

	virtual ~Metadata() {}
};

class ReadMetadata : public Metadata {
private:
	// Variables for DataNode
	std::string local_file_name;
public:
	// Variables for NameNode (mainly used to help with read/replica repair)
	bool replica_repair;											// is the read part of a replica repair task?
	std::unordered_map<uint32_t, int32_t> responses;				// key = data node id and value = the update counter the data node returned (value is < 0 if data node returned error)
	uint32_t highest_update_counter;								// highest update counter recorded in any response to this request
	WriteSuccessReadResponse* read_repair_source;					// response from highest update counter; used as source when sending read repair
	bool read_repair_needed;

	ReadMetadata(const std::string& local_file_name) : Metadata(), local_file_name(local_file_name), replica_repair(false), highest_update_counter(0), 
														read_repair_needed(false) {}

	ReadMetadata(int client_fd) : Metadata(client_fd), local_file_name(""), replica_repair(false), highest_update_counter(0), read_repair_needed(false) {}

	ReadMetadata& operator=(const ReadMetadata& other) = delete;
	ReadMetadata(const ReadMetadata& other) = delete;

	~ReadMetadata() override {
	};

	std::string get_local_file_name() {
		return this->local_file_name;
	}

};

class WriteMetadata : public Metadata {
private:
	// Variables for DataNode/NameNode for read repair
    std::string local_file_name;
public:
	// Variables for NameNode (mainly used to help with read repair)
	bool read_repair;								// is the write part of a replica repair task?
	std::unordered_set<uint32_t> read_repair_on_replicas;	// list of replicas that are affected by the read_repair
	uint32_t num_read_replica_responses;			// number of replicas (that were affected by the read_repair) that have either ACKed the write or
													// we detected they have failed

    WriteMetadata(std::string local_file_name) : Metadata(), local_file_name(local_file_name), read_repair(false), num_read_replica_responses(0) {}
	WriteMetadata(int client_fd, bool read_repair) : Metadata(client_fd), read_repair(read_repair), num_read_replica_responses(0) {
		assert(not read_repair);
	}
	WriteMetadata(bool read_repair) :  Metadata(), read_repair(read_repair), num_read_replica_responses(0) {
		assert(read_repair);
	}

	WriteMetadata& operator=(const WriteMetadata& other) = delete;
	WriteMetadata(const WriteMetadata& other) = delete;

	~WriteMetadata() override {};

	std::string get_local_file_name() {
		return this->local_file_name;
	}

	void set_local_file_name(std::string file_name) {
		this->local_file_name = file_name;
	}
};
