#include <arpa/inet.h>
#include <sys/stat.h>

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <string>
#include <iostream>

#include "data_node_block_report_manager.hpp"

static const std::string file_system_root_dir = "test_block_report_filesystem_dir";

uint32_t init_block_report(BlockReport& block_report_manager) {
	block_report_manager.add_new_file("file0");
        block_report_manager.add_new_file("file1");
        block_report_manager.add_new_file("file2");

        assert(block_report_manager.set_file_update_counter("file0", 0) == 0);
        assert(block_report_manager.set_file_update_counter("file1", 10) == 0);
        assert(block_report_manager.set_file_update_counter("file2", 20) == 0);

	assert(block_report_manager.get_update_counter() == 6);

	return (2*sizeof(uint32_t)) + (3 * (sizeof(uint32_t) + 6));
}

void test_serializing_block_report() {
	BlockReport block_report_manager(file_system_root_dir);
	uint32_t expected_serialized_size = init_block_report(block_report_manager);

	assert(block_report_manager.struct_serialized_size() == expected_serialized_size);
	char* buffer = new char[expected_serialized_size];
	assert(block_report_manager.serialize(buffer) - buffer == expected_serialized_size);

	uint32_t buffer_index = 0;

	uint32_t struct_size = 0;
	memcpy((char*)&struct_size, buffer, sizeof(uint32_t));
        buffer_index += sizeof(uint32_t);
	assert(ntohl(struct_size) == expected_serialized_size);

	uint32_t update_counter = 0;
	memcpy((char*)&update_counter, buffer + buffer_index, sizeof(uint32_t));
        buffer_index += sizeof(uint32_t);
	assert(ntohl(update_counter) == 6);

	bool found_files[3] = {false, false, false};
	for (uint32_t i = 0; i < 3; ++i) {
		std::string file_name(buffer + buffer_index);
		buffer_index += file_name.length() + 1;

		int file_id = file_name.back() - '0';
		found_files[file_id] = true;

		uint32_t file_counter;
		memcpy((char*)&file_counter, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		assert(ntohl(file_counter) == file_id * 10);
	}
	for (uint32_t i = 0; i < 3; ++i) {
		assert(found_files[i]);
	}

	delete buffer;
}

void test_deserializing_block_report() {
	BlockReport block_report_manager(file_system_root_dir);
	uint32_t expected_serialized_size = init_block_report(block_report_manager);

        assert(block_report_manager.struct_serialized_size() == expected_serialized_size);
        char* buffer = new char[expected_serialized_size];
        assert(block_report_manager.serialize(buffer) - buffer == expected_serialized_size);

	BlockReport block_report_deserialized;
	block_report_deserialized.deserialize(buffer, &block_report_deserialized);
	assert(block_report_manager == block_report_deserialized);
}

void test_merging_block_report() {
	BlockReport block_report_manager(file_system_root_dir);
	uint32_t expected_serialized_size = init_block_report(block_report_manager);

        assert(block_report_manager.struct_serialized_size() == expected_serialized_size);
        char* buffer = new char[expected_serialized_size];
        assert(block_report_manager.serialize(buffer) - buffer == expected_serialized_size);

        BlockReport block_report_deserialized;
        block_report_deserialized.deserialize(buffer, &block_report_deserialized);
	assert(block_report_manager == block_report_deserialized);

	block_report_deserialized.set_file_update_counter("file2", 25);
	assert(block_report_manager != block_report_deserialized);

	block_report_manager.merge(&block_report_deserialized);
	assert(block_report_manager == block_report_deserialized);
}

int main() {
	mkdir(file_system_root_dir.c_str(), 0700);

	test_serializing_block_report();
	test_deserializing_block_report();
	test_merging_block_report();

	return 0;
}
