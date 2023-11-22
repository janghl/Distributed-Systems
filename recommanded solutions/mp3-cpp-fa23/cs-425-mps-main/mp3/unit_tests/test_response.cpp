#include <arpa/inet.h>
#include <sys/stat.h>

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <string>
#include <iostream>

#include "response.hpp"
#include "utils.hpp"
#include "test_includes.hpp"

static const std::string sdfs_write_file_name = "sdfs_dummy_write_file.txt";
static const std::string sdfs_read_file_name = "sdfs_dummy_read_file.txt";
static char* sdfs_read_data = "abcsj\0\4abcabcabc\132jfjfk";
static const uint32_t sdfs_read_data_length = 5 + 1 + 1 + (3*3) + 1 + 5;
static const uint32_t read_update_counter = 31;
static const uint32_t write_update_counter = 54;
static const uint32_t node_id = 10;
static const std::unordered_set<uint32_t> ls_replicas({5, 2});

Response init_response(uint32_t& serialzied_size) {
	serialzied_size = (4 * sizeof(uint32_t)) + sdfs_write_file_name.length();
	return Response(node_id, OperationType::DELETE, 0, sdfs_write_file_name);
}

WriteSuccessReadResponse init_success_read_response(uint32_t& serialzied_size) {
	serialzied_size = (6 * sizeof(uint32_t)) + sdfs_read_file_name.length() + sdfs_read_data_length;
	char* data = new char[sdfs_read_data_length];
        memcpy(data, sdfs_read_data, sdfs_read_data_length);
        return WriteSuccessReadResponse(node_id, OperationType::READ, 0, sdfs_read_file_name, read_update_counter, sdfs_read_data_length, data);
}

WriteSuccessReadResponse init_write_response(uint32_t& serialzied_size) {
	serialzied_size = (5 * sizeof(uint32_t)) + sdfs_write_file_name.length();
        return WriteSuccessReadResponse(node_id, OperationType::WRITE, 0, sdfs_write_file_name, write_update_counter);
}

LSResponse init_ls_response(uint32_t& serialzied_size) {
	serialzied_size = (5 * sizeof(uint32_t)) + sdfs_write_file_name.length() + (2 * sizeof(uint32_t));
	return LSResponse(node_id, OperationType::LS, 0, sdfs_write_file_name, ls_replicas);
}

void test_serializing_response() {
	uint32_t expected_serialized_size;
	Response response = init_response(expected_serialized_size);

	assert(response.struct_serialized_size() == expected_serialized_size);
	char* serialized_buffer = nullptr;
	assert(response.serialize(serialized_buffer) == expected_serialized_size);
	char* buffer = serialized_buffer;

	uint32_t buffer_index = 0;

	uint32_t network_node_id;
        memcpy((char*)&network_node_id, buffer, sizeof(uint32_t));
	assert(ntohl(network_node_id) == node_id);
	buffer += sizeof(uint32_t);

	uint32_t network_response_type;
        memcpy((char*)&network_response_type, buffer, sizeof(uint32_t));
	assert(static_cast<OperationType>(ntohl(network_response_type)) == OperationType::DELETE);
	buffer += sizeof(uint32_t);

	uint32_t network_status_code;
        memcpy((char*)&network_status_code, buffer, sizeof(uint32_t));
	assert(ntohl(network_status_code) == 0);
	buffer += sizeof(uint32_t);

	uint32_t file_name_len;
	memcpy((char*)&file_name_len, buffer, sizeof(uint32_t));
	file_name_len = ntohl(file_name_len);
	assert(file_name_len == sdfs_write_file_name.length());
	buffer += sizeof(uint32_t);

	char* file_name_buffer = new char[file_name_len + 1];
	file_name_buffer[file_name_len] = '\0';
	memcpy(file_name_buffer, buffer, file_name_len);
	assert(std::string(file_name_buffer) == sdfs_write_file_name);
	delete[] file_name_buffer;

	delete[] serialized_buffer;
}

void test_deserializing_response() {
	uint32_t expected_serialized_size;
        Response response = init_response(expected_serialized_size);

	assert(response.struct_serialized_size() == expected_serialized_size);
	char* buffer = nullptr;
	assert(response.serialize(buffer) == expected_serialized_size);

	Response deserialized = Response::deserialize(buffer);
	assert(response == deserialized);

	delete[] buffer;
}

void test_deserializing_read_response() {
        uint32_t expected_serialized_size;
        WriteSuccessReadResponse response = init_success_read_response(expected_serialized_size);

        assert(response.struct_serialized_size() == expected_serialized_size);
        char* buffer = nullptr;
        assert(response.serialize(buffer) == expected_serialized_size);

        WriteSuccessReadResponse deserialized = WriteSuccessReadResponse::deserialize(buffer);
        assert(response == deserialized);

        delete[] buffer;
}

void test_deserializing_ls_response() {
        uint32_t expected_serialized_size;
        LSResponse response = init_ls_response(expected_serialized_size);

        assert(response.struct_serialized_size() == expected_serialized_size);
        char* buffer = nullptr;
        assert(response.serialize(buffer) == expected_serialized_size);

        LSResponse deserialized = LSResponse::deserialize(buffer);
        assert(response == deserialized);

        delete[] buffer;
}

void test_deserializing_write_response() {
        uint32_t expected_serialized_size;
        WriteSuccessReadResponse response = init_write_response(expected_serialized_size);

        assert(response.struct_serialized_size() == expected_serialized_size);
        char* buffer = nullptr;
        assert(response.serialize(buffer) == expected_serialized_size);

        WriteSuccessReadResponse deserialized = WriteSuccessReadResponse::deserialize(buffer);
        assert(response == deserialized);

        delete[] buffer;
}

void test_serializing_read_response() {
	uint32_t expected_serialized_size;
	WriteSuccessReadResponse response = init_success_read_response(expected_serialized_size);

	assert(response.struct_serialized_size() == expected_serialized_size);
	char* serialized_buffer = nullptr;
	assert(response.serialize(serialized_buffer) == expected_serialized_size);
	char* buffer = serialized_buffer;

	uint32_t buffer_index = 0;

	uint32_t network_node_id;
        memcpy((char*)&network_node_id, buffer, sizeof(uint32_t));
        assert(ntohl(network_node_id) == node_id);
        buffer += sizeof(uint32_t);

        uint32_t network_response_type;
        memcpy((char*)&network_response_type, buffer, sizeof(uint32_t));
        assert(static_cast<OperationType>(ntohl(network_response_type)) == OperationType::READ);
        buffer += sizeof(uint32_t);

        uint32_t network_status_code;
        memcpy((char*)&network_status_code, buffer, sizeof(uint32_t));
        assert(ntohl(network_status_code) == 0);
        buffer += sizeof(uint32_t);

        uint32_t file_name_len;
        memcpy((char*)&file_name_len, buffer, sizeof(uint32_t));
        file_name_len = ntohl(file_name_len);
        assert(file_name_len == sdfs_read_file_name.length());
        buffer += sizeof(uint32_t);

        char* file_name_buffer = new char[file_name_len + 1];
        file_name_buffer[file_name_len] = '\0';
        memcpy(file_name_buffer, buffer, file_name_len);
        assert(std::string(file_name_buffer) == sdfs_read_file_name);
        delete[] file_name_buffer;
	buffer += file_name_len;

	uint32_t network_update_counter;
       	memcpy((char*)&network_update_counter, buffer, sizeof(uint32_t));
	assert(ntohl(network_update_counter) == read_update_counter);
       	buffer += sizeof(uint32_t);

       	uint32_t data_len;
       	memcpy((char*)&data_len, buffer, sizeof(uint32_t));
	data_len = ntohl(data_len);
	assert(data_len == sdfs_read_data_length);
       	buffer += sizeof(uint32_t);

       	char* data = new char[data_len];
	memcpy(data, buffer, data_len);
	assert(compare_data(data, sdfs_read_data, sdfs_read_data_length) == true);

	delete[] serialized_buffer;
}

void test_serializing_ls_response() {
	uint32_t expected_serialized_size;
        LSResponse response = init_ls_response(expected_serialized_size);

        assert(response.struct_serialized_size() == expected_serialized_size);
        char* serialized_buffer = nullptr;
        assert(response.serialize(serialized_buffer) == expected_serialized_size);
        char* buffer = serialized_buffer;

        uint32_t buffer_index = 0;

	uint32_t network_node_id;
        memcpy((char*)&network_node_id, buffer, sizeof(uint32_t));
        assert(ntohl(network_node_id) == node_id);
        buffer += sizeof(uint32_t);

        uint32_t network_response_type;
        memcpy((char*)&network_response_type, buffer, sizeof(uint32_t));
        assert(static_cast<OperationType>(ntohl(network_response_type)) == OperationType::LS);
        buffer += sizeof(uint32_t);

        uint32_t network_status_code;
        memcpy((char*)&network_status_code, buffer, sizeof(uint32_t));
        assert(ntohl(network_status_code) == 0);
        buffer += sizeof(uint32_t);

        uint32_t file_name_len;
        memcpy((char*)&file_name_len, buffer, sizeof(uint32_t));
        file_name_len = ntohl(file_name_len);
        assert(file_name_len == sdfs_write_file_name.length());
        buffer += sizeof(uint32_t);

        char* file_name_buffer = new char[file_name_len + 1];
        file_name_buffer[file_name_len] = '\0';
        memcpy(file_name_buffer, buffer, file_name_len);
        assert(std::string(file_name_buffer) == sdfs_write_file_name);
        delete[] file_name_buffer;
        buffer += file_name_len;

        uint32_t network_num_replicas;
        memcpy((char*)&network_num_replicas, buffer, sizeof(uint32_t));
        assert(ntohl(network_num_replicas) == ls_replicas.size());
        buffer += sizeof(uint32_t);

	for (int i = 0; i < ls_replicas.size(); ++i) {
                uint32_t network_replica_id;
                memcpy((char*)&network_replica_id, buffer, sizeof(uint32_t));
                assert(ls_replicas.find(ntohl(network_replica_id)) != ls_replicas.end());
                buffer += sizeof(uint32_t);
	}

        delete[] serialized_buffer;

}

void test_serializing_write_response() {
	uint32_t expected_serialized_size;
	WriteSuccessReadResponse response = init_write_response(expected_serialized_size);

	assert(response.struct_serialized_size() == expected_serialized_size);
	char* serialized_buffer = nullptr;
	assert(response.serialize(serialized_buffer) == expected_serialized_size);
	char* buffer = serialized_buffer;

	uint32_t buffer_index = 0;

	uint32_t network_node_id;
        memcpy((char*)&network_node_id, buffer, sizeof(uint32_t));
        assert(ntohl(network_node_id) == node_id);
        buffer += sizeof(uint32_t);

        uint32_t network_response_type;
        memcpy((char*)&network_response_type, buffer, sizeof(uint32_t));
        assert(static_cast<OperationType>(ntohl(network_response_type)) == OperationType::WRITE);
        buffer += sizeof(uint32_t);

        uint32_t network_status_code;
        memcpy((char*)&network_status_code, buffer, sizeof(uint32_t));
        assert(ntohl(network_status_code) == 0);
        buffer += sizeof(uint32_t);

        uint32_t file_name_len;
        memcpy((char*)&file_name_len, buffer, sizeof(uint32_t));
        file_name_len = ntohl(file_name_len);
        assert(file_name_len == sdfs_write_file_name.length());
        buffer += sizeof(uint32_t);

        char* file_name_buffer = new char[file_name_len + 1];
        file_name_buffer[file_name_len] = '\0';
        memcpy(file_name_buffer, buffer, file_name_len);
        assert(std::string(file_name_buffer) == sdfs_write_file_name);
        delete[] file_name_buffer;
	buffer += file_name_len;

	uint32_t network_update_counter;
       	memcpy((char*)&network_update_counter, buffer, sizeof(uint32_t));
	assert(ntohl(network_update_counter) == write_update_counter);
       	buffer += sizeof(uint32_t);

	delete[] serialized_buffer;
}

int main() {
	test_serializing_response();
	test_serializing_read_response();
	test_serializing_write_response();
	test_serializing_ls_response();

	test_deserializing_response();
	test_deserializing_read_response();
	test_deserializing_write_response();
        test_deserializing_ls_response();

	return 0;
}
