#include <arpa/inet.h>
#include <sys/stat.h>

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <string>
#include <iostream>

#include "request.hpp"
#include "utils.hpp"

std::string machine_id;
std::vector<std::string> machine_ips;
int grep_server_port, server_port;

static const std::string sdfs_read_file_name = "sdfs_dummy_read_file.txt";
Request init_read_request(uint32_t& serialized_size) {
	ReadMetadata* read_meta = new ReadMetadata("dummy_read_file.txt");
	serialized_size = sizeof(uint32_t) + sizeof(uint32_t) + sdfs_read_file_name.length();
	return Request(OperationType::READ, sdfs_read_file_name, read_meta);
}

static const std::string sdfs_write_file_name = "sdfs_dummy_write_file.txt";
static char* sdfs_write_data = "abcsj\0\4abcabcabc\132jfjfk";
static const uint32_t sdfs_write_data_length = 5 + 1 + 1 + (3*3) + 1 + 5;
static const uint32_t write_update_counter = 31;
Request init_write_request(uint32_t& serialized_size) {
	WriteMetadata* write_meta = new WriteMetadata("dummy_write_file.txt");
	serialized_size = sizeof(uint32_t) + sizeof(uint32_t) + sdfs_write_file_name.length() + sizeof(uint32_t) + 
		sizeof(uint32_t) + sdfs_write_data_length;
	char* data = new char[sdfs_write_data_length];
	memcpy(data, sdfs_write_data, sdfs_write_data_length);
	return Request(OperationType::WRITE, sdfs_write_file_name, data, sdfs_write_data_length, write_update_counter, write_meta);
}

void test_serializing_read_request() {
	uint32_t expected_serialized_size;
	Request read_request = init_read_request(expected_serialized_size);

	assert(read_request.struct_serialized_size() == expected_serialized_size);
	char* serialized_buffer = nullptr;
	assert(read_request.serialize(serialized_buffer) == expected_serialized_size);
	char* buffer = serialized_buffer;

	uint32_t buffer_index = 0;

	uint32_t network_request_type;
        memcpy((char*)&network_request_type, buffer, sizeof(uint32_t));
	assert(static_cast<OperationType>(ntohl(network_request_type)) == OperationType::READ);
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

	delete[] serialized_buffer;
}

void test_deserializing_read_request() {
	uint32_t expected_serialized_size;
        Request read_request = init_read_request(expected_serialized_size);

	assert(read_request.struct_serialized_size() == expected_serialized_size);
	char* buffer = nullptr;
	assert(read_request.serialize(buffer) == expected_serialized_size);

	Request read_deserialized = Request::deserialize(buffer);
	assert(read_request == read_deserialized);

	delete[] buffer;
}

void test_serializing_write_request() {
	uint32_t expected_serialized_size;
	Request write_request = init_write_request(expected_serialized_size);

	assert(write_request.struct_serialized_size() == expected_serialized_size);
	char* serialized_buffer = nullptr;
	assert(write_request.serialize(serialized_buffer) == expected_serialized_size);
	char* buffer = serialized_buffer;

	uint32_t buffer_index = 0;

	uint32_t network_request_type;
	memcpy((char*)&network_request_type, buffer, sizeof(uint32_t));
	assert(static_cast<OperationType>(ntohl(network_request_type)) == OperationType::WRITE);
	buffer += sizeof(uint32_t);

	uint32_t file_name_len;
	memcpy((char*)&file_name_len, buffer, sizeof(uint32_t));
	file_name_len = ntohl(file_name_len);
	assert(file_name_len == sdfs_write_file_name.length());
	buffer += sizeof(uint32_t);

	char* file_name_buffer = new char[file_name_len + 1];
	file_name_buffer[file_name_len] = '\0';
	memcpy(file_name_buffer, buffer, file_name_len);
	std::string file_name_str(file_name_buffer);
	delete[] file_name_buffer;
	assert(file_name_str == sdfs_write_file_name);
	buffer += file_name_len;
	assert(file_name_str.length() == file_name_len);

	uint32_t network_update_counter;
	memcpy((char*)&network_update_counter, buffer, sizeof(uint32_t));
	assert(ntohl(network_update_counter) == write_update_counter);
	buffer += sizeof(uint32_t);

	uint32_t data_len;
	memcpy((char*)&data_len, buffer, sizeof(uint32_t));
	data_len = ntohl(data_len);
	assert(data_len == sdfs_write_data_length);
	buffer += sizeof(uint32_t);

	char* data = new char[data_len];
	memcpy(data, buffer, data_len);
	assert(compare_data(data, sdfs_write_data, sdfs_write_data_length) == true);

	delete[] serialized_buffer;
}

void test_deserializing_write_request() {
	uint32_t expected_serialized_size;
        Request write_request = init_write_request(expected_serialized_size);

	assert(write_request.struct_serialized_size() == expected_serialized_size);
	char* buffer = nullptr;
	assert(write_request.serialize(buffer) == expected_serialized_size);

	Request write_deserialized = Request::deserialize(buffer);
	assert(write_request == write_deserialized);

	delete[] buffer;
}

int main() {
	test_serializing_read_request();
	test_serializing_write_request();
	test_deserializing_read_request();
	test_deserializing_write_request();

	return 0;
}
