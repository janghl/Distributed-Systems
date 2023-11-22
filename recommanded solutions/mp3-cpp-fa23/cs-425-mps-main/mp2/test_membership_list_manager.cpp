#include <arpa/inet.h>

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <string>
#include <iostream>

#include "membership_list_manager.hpp"
#include "utils.hpp"

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

void test_serializing_membership_list() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0;
	MembershipListManager membership_list_manager(machine_ids, machine_ids[self_machine_index], 0);

	uint32_t buffer_size = 0;
	char* buffer = membership_list_manager.serialize_membership_list(buffer_size, true);

	assert(buffer_size == (sizeof(uint32_t) * 5 * machine_ids.size()));
	uint32_t buffer_index = 0;
	for (uint32_t i = 0; i < machine_ids.size(); ++i) {
		uint32_t machine_id, machine_version, machine_heartbeat, machine_incarnation, machine_state;

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

		assert(ntohl(machine_id) == machine_ids[i]);
		assert(ntohl(machine_version) == 0);
		assert(ntohl(machine_heartbeat) == 0);
		assert(ntohl(machine_incarnation) == 0);
		assert(ntohl(machine_state) == MachineState::Alive);
	}

	delete buffer;
}

void test_purging_of_failed_nodes() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0, self_machine_version = 1;
	uint32_t self_machine_id = machine_ids[self_machine_index];
	MembershipListManager membership_list_manager(machine_ids, self_machine_id, self_machine_version);
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
}


void test_serializing_membership_list_with_failed_nodes() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0;
	uint32_t self_machine_version = 1;
        uint32_t self_machine_id = machine_ids[self_machine_index];
	MembershipListManager membership_list_manager(machine_ids, self_machine_id, self_machine_version);
	usleep(1000); // sleep for 1 milli second

	membership_list_manager.self_increment_heartbeat_counter();
	membership_list_manager.update_membership_list(0, 1, 10000, false);

	uint32_t buffer_size = 0;
	assert(membership_list_manager.get_membership_list().size() == machine_ids.size() - 1);
	char* buffer = membership_list_manager.serialize_membership_list(buffer_size, true);

	assert(buffer_size == (sizeof(uint32_t) * 5));
	uint32_t buffer_index = 0;
	for (uint32_t i = 0; i < 1; ++i) {
		uint32_t machine_id, machine_version, machine_heartbeat, machine_incarnation, machine_state;

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

		assert(ntohl(machine_id) == machine_ids[i]);
		assert(ntohl(machine_incarnation) == 0);

		assert((i != self_machine_index && ntohl(machine_version) == 0) || 
				(i == self_machine_index && ntohl(machine_version) == 1));
		assert((i != self_machine_index && ntohl(machine_heartbeat) == 0) || 
				(i == self_machine_index && ntohl(machine_heartbeat) == 1));
		assert((i != self_machine_index && ntohl(machine_state) == MachineState::Failed) || 
				(i == self_machine_index && ntohl(machine_state) == MachineState::Alive));
	}

	delete buffer;
}

void test_deserializing_membership_list() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0;
	uint32_t self_machine_version = 0;
        uint32_t self_machine_id = machine_ids[self_machine_index];
	MembershipListManager membership_list_manager(machine_ids, self_machine_id, self_machine_version);

	uint32_t buffer_size = 0;
	char* buffer = membership_list_manager.serialize_membership_list(buffer_size, true);
	assert(buffer_size == (sizeof(uint32_t) * 5 * machine_ids.size()));

	uint32_t last_index = machine_ids.size() - 1;
	uint32_t last_member_id = machine_ids[last_index];
	MembershipListManager membership_list_manager_deserialized(last_member_id, 0);

	std::vector<MemberInformation> deserialized_member_info = 
		membership_list_manager_deserialized.deserialize_membership_list(buffer, buffer_size, true);
	membership_list_manager_deserialized.merge_membership_list(deserialized_member_info);

	delete buffer;

	const std::vector<MemberInformation>& membership_list_deserialized = membership_list_manager_deserialized.get_membership_list();
	assert(membership_list_deserialized.size() == last_index);
	for (uint32_t i = 0; i < last_index; ++i) {
		const MemberInformation& member = membership_list_deserialized[i];
		assert(machine_ids[i] == member.get_machine_id());
		assert(0 == member.get_machine_version());
		assert(get_machine_ip(machine_ids[i]) == member.get_machine_ip());
		assert(0 == member.get_heartbeat_counter());
		assert(not member.has_failed());
	}

	const MemberInformation& member = membership_list_manager_deserialized.self_get_status();
	assert(last_member_id == member.get_machine_id());
	assert(get_machine_ip(last_member_id) == member.get_machine_ip());
	assert(0 == member.get_machine_version());
	assert(0 == member.get_heartbeat_counter());
	assert(not member.has_failed());
}

void test_merging_membership_list() {
	std::vector<int> machine_ids = {1, 2, 3};
	uint32_t self_machine_index = 0;
        uint32_t self_machine_id = machine_ids[self_machine_index];
        uint32_t self_machine_version = 0;
	MembershipListManager membership_list_manager_1(machine_ids, self_machine_id, self_machine_version);

	uint32_t membership_list_manager_2_self_index = 1;
	uint32_t membership_list_manager_2_self_version = 0;
	uint32_t membership_list_manager_2_self_id = machine_ids[membership_list_manager_2_self_index];
	MembershipListManager membership_list_manager_2(machine_ids, membership_list_manager_2_self_id, membership_list_manager_2_self_version);

	membership_list_manager_1.self_increment_heartbeat_counter();
	membership_list_manager_2.self_increment_heartbeat_counter();

	uint32_t buffer_size = 0;
	assert(membership_list_manager_1.get_membership_list().size() == machine_ids.size() - 1);
	char* buffer = membership_list_manager_1.serialize_membership_list(buffer_size, true);
	assert(buffer_size == (sizeof(uint32_t) * 5 * machine_ids.size()));

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
		}
	}

	const MemberInformation& member = membership_list_manager_2.self_get_status();
	assert(membership_list_manager_2_self_id == member.get_machine_id());
	assert(get_machine_ip(membership_list_manager_2_self_id) == member.get_machine_ip());
	assert(0 == member.get_machine_version());
	assert(not member.has_failed());
	assert(1 == member.get_heartbeat_counter());
}


int main() {
	test_serializing_membership_list();
	test_deserializing_membership_list();
	test_merging_membership_list();
	test_purging_of_failed_nodes();
	test_serializing_membership_list_with_failed_nodes();

	return 0;
}
