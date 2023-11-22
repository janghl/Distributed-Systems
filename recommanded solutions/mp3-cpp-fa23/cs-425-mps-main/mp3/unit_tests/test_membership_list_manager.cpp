#include <arpa/inet.h>

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <string>
#include <iostream>

#include "data_node_block_report_manager.hpp"

#include "../mp2/membership_list_manager.hpp"
#include "../mp2/utils.hpp"

int server_port;
std::mutex protocol_mutex;
membership_protocol_options membership_protocol  = membership_protocol_options::GOSSIP;
int generation_count = 0;
std::vector<std::string> machine_ips;

uint64_t bytes_sent; 
uint64_t bytes_received;
uint64_t failures = 0;
uint64_t num_gossips = 0;

MembershipListManager membership_list_manager;

#define expected_block_report_header_size (2 * sizeof(uint32_t))
static constexpr uint32_t expected_block_report_update_counter = 6;

uint32_t get_exepected_membership_list_serialize_size(uint32_t total_machines, uint32_t expected_block_report_file_metadata_size) {
	return (((sizeof(uint32_t) * 5) + expected_block_report_header_size) * total_machines) +
                                                expected_block_report_file_metadata_size;
}

uint32_t init_block_report(BlockReport& block, int base_index = 0) {
	block.add_new_file("file" + std::to_string(base_index));
        block.add_new_file("file" + std::to_string(base_index + 1));
        block.add_new_file("file" + std::to_string(base_index + 2));

        assert(block.set_file_update_counter("file" + std::to_string(base_index), base_index * 10) == 0);
        assert(block.set_file_update_counter("file" + std::to_string(base_index + 1), base_index * 10 + 10) == 0);
        assert(block.set_file_update_counter("file" + std::to_string(base_index + 2), base_index * 10 + 20) == 0);
	assert(block.get_update_counter() == expected_block_report_update_counter);

	return 3 * (sizeof(uint32_t) + 6);
}

uint32_t init_self_block_report(MembershipListManager& membership_list) {
	BlockReport* block_report_manager = static_cast<BlockReport*>(membership_list.get_self_extra_information());
	return init_block_report(*block_report_manager, 0);
}

void verify_block_report_file_metadata(char* buffer, uint32_t& buffer_index, int base_index = 0) {
	bool found_files[3] = {false, false, false};
	for (uint32_t j = 0; j < 3; ++j) {
		std::string file_name(buffer + buffer_index);
		buffer_index += file_name.length() + 1;

		int file_id = file_name.back() - '0';
		found_files[file_id - base_index] = true;

		uint32_t file_counter;
		memcpy((char*)&file_counter, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		assert(ntohl(file_counter) == file_id * 10);
       	}
       	for (uint32_t j = 0; j < 3; ++j) {
       		assert(found_files[j]);
       	}
}

void test_serializing_membership_list() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0;
	MembershipListManager membership_list_manager(machine_ids, machine_ids[self_machine_index], 0);

	uint32_t expected_block_report_file_metadata_size = init_self_block_report(membership_list_manager);
	uint32_t expected_serialized_size = get_exepected_membership_list_serialize_size(machine_ids.size(), 
						expected_block_report_file_metadata_size);
	uint32_t buffer_size = 0;
	char* buffer = membership_list_manager.serialize_membership_list(buffer_size, true);

	assert(buffer_size == expected_serialized_size);
	uint32_t buffer_index = 0;
	for (uint32_t i = 0; i < machine_ids.size(); ++i) {
		uint32_t machine_id, machine_version, machine_heartbeat, machine_incarnation, machine_state, block_report_size, 
			 block_report_update_counter;

		memcpy((char*)&machine_id, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&machine_version, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&machine_heartbeat, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&machine_incarnation, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&machine_state, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&block_report_size, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&block_report_update_counter, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);

		assert(ntohl(machine_id) == machine_ids[i]);
		assert(ntohl(machine_version) == 0);
		assert(ntohl(machine_heartbeat) == 0);
		assert(ntohl(machine_incarnation) == 0);
		assert(ntohl(machine_state) == MachineState::Alive);
		assert(ntohl(block_report_size) == expected_block_report_header_size + (i == 0 ? expected_block_report_file_metadata_size : 0));
		assert(ntohl(block_report_update_counter) == (i == 0 ? expected_block_report_update_counter : 0));
		if (i == 0) {
			verify_block_report_file_metadata(buffer, buffer_index);
		}
	}

	delete buffer;
}

void test_purging_of_failed_nodes() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0, self_machine_version = 1;
	uint32_t self_machine_id = machine_ids[self_machine_index];
	MembershipListManager membership_list_manager(machine_ids, self_machine_id, self_machine_version);
	init_self_block_report(membership_list_manager);
	usleep(2000); // sleep for 2 milli second

	membership_list_manager.self_increment_heartbeat_counter();
	membership_list_manager.update_membership_list(0, 1, 1, false);

	const std::vector<MemberInformation>& membership_list = membership_list_manager.get_membership_list();
	assert(membership_list.size() == 0);

	const MemberInformation& member = membership_list_manager.self_get_status();
	assert(self_machine_id == member.get_machine_id());
	assert(self_machine_version == member.get_machine_version());
	assert(get_machine_ip(self_machine_id) == member.get_machine_ip());
	assert(1 == member.get_heartbeat_counter());
	assert(not member.has_failed());
	BlockReport expected_block;
	init_block_report(expected_block, 0);
	assert(*member.get_extra_information() == expected_block);
}


void test_serializing_membership_list_with_failed_nodes() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0;
	uint32_t self_machine_version = 1;
	uint32_t self_machine_id = machine_ids[self_machine_index];
	MembershipListManager membership_list_manager(machine_ids, self_machine_id, self_machine_version);

	uint32_t expected_block_report_file_metadata_size = init_self_block_report(membership_list_manager);

	usleep(1000); // sleep for 1 milli second

	membership_list_manager.self_increment_heartbeat_counter();
	membership_list_manager.update_membership_list(0, 1, 10000, false);

	assert(membership_list_manager.get_membership_list().size() == machine_ids.size() - 1);
	uint32_t expected_serialized_size = get_exepected_membership_list_serialize_size(1, 
						expected_block_report_file_metadata_size);
	uint32_t buffer_size = 0;
	char* buffer = membership_list_manager.serialize_membership_list(buffer_size, true);

	assert(buffer_size == expected_serialized_size);
	uint32_t buffer_index = 0;
	for (uint32_t i = 0; i < 1; ++i) {
		uint32_t machine_id, machine_version, machine_heartbeat, machine_incarnation, machine_state, block_report_size, 
			 block_report_update_counter;

		memcpy((char*)&machine_id, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&machine_version, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&machine_heartbeat, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&machine_incarnation, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&machine_state, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&block_report_size, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);
		memcpy((char*)&block_report_update_counter, buffer + buffer_index, sizeof(uint32_t));
		buffer_index += sizeof(uint32_t);

		assert(ntohl(machine_id) == machine_ids[i]);
		assert(ntohl(machine_incarnation) == 0);

		assert((i != self_machine_index && ntohl(machine_version) == 0) || 
				(i == self_machine_index && ntohl(machine_version) == 1));
		assert((i != self_machine_index && ntohl(machine_heartbeat) == 0) || 
				(i == self_machine_index && ntohl(machine_heartbeat) == 1));
		assert((i != self_machine_index && ntohl(machine_state) == MachineState::Failed) || 
				(i == self_machine_index && ntohl(machine_state) == MachineState::Alive));
		assert(ntohl(block_report_size) == expected_block_report_header_size + 
				(i == self_machine_index ? expected_block_report_file_metadata_size : 0));
		assert(ntohl(block_report_update_counter) == (i == self_machine_index ? expected_block_report_update_counter : 0));
		if (i == self_machine_index) {
			verify_block_report_file_metadata(buffer, buffer_index);
		}
	}

	delete buffer;
}

void test_deserializing_membership_list() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0;
	uint32_t self_machine_version = 0;
	uint32_t self_machine_id = machine_ids[self_machine_index];
	MembershipListManager membership_list_manager(machine_ids, self_machine_id, self_machine_version);

	uint32_t expected_block_report_file_metadata_size = init_self_block_report(membership_list_manager);
	uint32_t expected_serialized_size = get_exepected_membership_list_serialize_size(machine_ids.size(), 
						expected_block_report_file_metadata_size);

	uint32_t buffer_size = 0;
	char* buffer = membership_list_manager.serialize_membership_list(buffer_size, true);
	assert(buffer_size == expected_serialized_size);

	uint32_t last_index = machine_ids.size() - 1;
	uint32_t last_member_id = machine_ids[last_index];
	MembershipListManager membership_list_manager_deserialized(last_member_id, 0);

	std::vector<MemberInformation> deserialized_member_info = 
		membership_list_manager_deserialized.deserialize_membership_list(buffer, buffer_size, true);
	membership_list_manager_deserialized.merge_membership_list(deserialized_member_info);

	delete buffer;

	const std::vector<MemberInformation>& membership_list_deserialized = membership_list_manager_deserialized.get_membership_list();
	const std::vector<MemberInformation>& expected_membership_list = membership_list_manager.get_membership_list();

	assert(membership_list_deserialized.size() == last_index);
	for (uint32_t i = 0; i < last_index; ++i) {
		const MemberInformation& member = membership_list_deserialized[i];
		assert(machine_ids[i] == member.get_machine_id());
		assert(0 == member.get_machine_version());
		assert(get_machine_ip(machine_ids[i]) == member.get_machine_ip());
		assert(0 == member.get_heartbeat_counter());
		assert(not member.has_failed());
		if (i == self_machine_index) {
			assert(member.get_extra_information()->operator==(*membership_list_manager.get_self_extra_information()));
		} else {
			assert(member.get_extra_information()->operator==(*expected_membership_list[i].get_extra_information()));
		}
	}

	const MemberInformation& member = membership_list_manager_deserialized.self_get_status();
	assert(last_member_id == member.get_machine_id());
	assert(get_machine_ip(last_member_id) == member.get_machine_ip());
	assert(0 == member.get_machine_version());
	assert(0 == member.get_heartbeat_counter());
	assert(not member.has_failed());
	if (last_index == self_machine_index) {
		assert(member.get_extra_information()->operator==(*membership_list_manager.get_self_extra_information()));
	} else {
		assert(member.get_extra_information()->operator==(*expected_membership_list[last_index - 1].get_extra_information()));
	}
}

void test_merging_membership_list() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0;
	uint32_t self_machine_id = machine_ids[self_machine_index];
	uint32_t self_machine_version = 0;
	MembershipListManager membership_list_manager_1(machine_ids, self_machine_id, self_machine_version);
	BlockReport* block_report_manager_1 = static_cast<BlockReport*>(membership_list_manager_1.get_self_extra_information());
        uint32_t block_report_manager_1_size = init_block_report(*block_report_manager_1, 0);
	uint32_t expected_serialized_size = get_exepected_membership_list_serialize_size(machine_ids.size(), 
						block_report_manager_1_size);

	uint32_t membership_list_manager_2_self_index = 1;
	uint32_t membership_list_manager_2_self_version = 0;
	uint32_t membership_list_manager_2_self_id = machine_ids[membership_list_manager_2_self_index];
	MembershipListManager membership_list_manager_2(machine_ids, membership_list_manager_2_self_id, membership_list_manager_2_self_version);
	BlockReport* block_report_manager_2 = static_cast<BlockReport*>(membership_list_manager_2.get_self_extra_information());
        init_block_report(*block_report_manager_2, 3);

	membership_list_manager_1.self_increment_heartbeat_counter();
	membership_list_manager_2.self_increment_heartbeat_counter();

	assert(membership_list_manager_1.get_membership_list().size() == machine_ids.size() - 1);
	uint32_t buffer_size = 0;
	char* buffer = membership_list_manager_1.serialize_membership_list(buffer_size, true);
	assert(buffer_size == expected_serialized_size);

	std::vector<MemberInformation> deserialized_member_info =
		membership_list_manager_2.deserialize_membership_list(buffer, buffer_size, true);
	membership_list_manager_2.merge_membership_list(deserialized_member_info);

	delete buffer;

	const std::vector<MemberInformation>& membership_list_2 = membership_list_manager_2.get_membership_list();
	assert(membership_list_2.size() == machine_ids.size() - 1);
	for (uint32_t i = 0; i < machine_ids.size(); ++i) {
		if (i == membership_list_manager_2_self_index) {
			continue;
		}
		const MemberInformation& member = membership_list_2[ (i > membership_list_manager_2_self_index) ? (i - 1) : i ];
		assert(machine_ids[i] == member.get_machine_id());
		assert(get_machine_ip(machine_ids[i]) == member.get_machine_ip());
		assert(0 == member.get_machine_version());
		assert(not member.has_failed());
		if (machine_ids[i] != self_machine_id) {
			assert(0 == member.get_heartbeat_counter());
		} else {
			assert(1 == member.get_heartbeat_counter());
			assert(*member.get_extra_information() == *block_report_manager_1);
		}
	}

	const MemberInformation& member = membership_list_manager_2.self_get_status();
	assert(membership_list_manager_2_self_id == member.get_machine_id());
	assert(get_machine_ip(membership_list_manager_2_self_id) == member.get_machine_ip());
	assert(0 == member.get_machine_version());
	assert(not member.has_failed());
	assert(1 == member.get_heartbeat_counter());
	assert(*member.get_extra_information() == *block_report_manager_2);
}


int main() {
	test_serializing_membership_list();
	test_deserializing_membership_list();
	test_merging_membership_list();
	test_purging_of_failed_nodes();
	test_serializing_membership_list_with_failed_nodes();

	return 0;
}
