#pragma once
#include <arpa/inet.h>

#include <cassert>
#include <iomanip>
#include <cstring>
#include <vector>
#include <mutex>
#include <string>

#include "utils.hpp"

enum MachineState {Failed = 0, Alive = 1, Suspected = 2, Leave = 3};
static constexpr int left_indent = 16;

static void print_header(bool print_incarnation) {
	std::cout << (print_incarnation ? "GOSSIP_SUSPECT" : "GOSSIP") << std::endl;
	std::cout << std::setw(left_indent) << std::fixed << "Machine Id" << "|";
	std::cout << std::setw(left_indent) << std::fixed << "Machine Version" << "|";
	std::cout << std::setw(left_indent) << std::fixed << "Heartbeat" << "|";
	if (print_incarnation) {
		std::cout << std::setw(left_indent) << std::fixed << "Incarnation" << "|";
	}
	std::cout << std::setw(left_indent) << std::fixed << "Update time" << "|";
	std::cout << std::setw(left_indent) << std::fixed << "Machine State" << "|";
	std::cout << std::endl;
}

class MemberInformation {
private:
	// constants identifying member
	uint32_t machine_id;
	uint32_t machine_version_number;
	std::string machine_ip;

	// most recent member information
	uint32_t heartbeat_counter;
	uint32_t incarnation_number;

	// member state set locally
	unsigned long last_update_time;
	MachineState machine_state;

public:
	MemberInformation() :
		heartbeat_counter(0),
		incarnation_number(0),
		last_update_time(0),
		machine_version_number(0),
		machine_state(MachineState::Alive) {}	

	MemberInformation(uint32_t machine_id, std::string machine_ip, uint32_t machine_version_number, uint32_t heartbeat_counter = 0, 
			unsigned long last_update_time = 0, uint32_t incarnation_number = 0) :
		machine_id(machine_id),
		machine_version_number(machine_version_number),
		machine_ip(machine_ip),
		heartbeat_counter(heartbeat_counter), 
		incarnation_number(incarnation_number), 
		last_update_time(last_update_time),
		machine_state(MachineState::Alive) {}

	MemberInformation(uint32_t machine_id, std::string machine_ip, uint32_t machine_version_number, const MemberInformation& other, 
			unsigned long last_update_time) :
		machine_id(machine_id),
		machine_version_number(machine_version_number),
		machine_ip(machine_ip),
		heartbeat_counter(other.heartbeat_counter),
		incarnation_number(other.incarnation_number),
		last_update_time(last_update_time),
		machine_state(MachineState::Alive) {}

	void init(uint32_t machine_id, std::string machine_ip, uint32_t machine_version_number, uint32_t heartbeat_counter = 0,
			unsigned long last_update_time = 0, uint32_t incarnation_number = 0) {
		this->machine_id = machine_id;
		this->machine_version_number = machine_version_number;
		this->machine_ip = machine_ip;
		this->heartbeat_counter = heartbeat_counter;
		this->incarnation_number = incarnation_number;
		this->last_update_time = last_update_time;
		this->machine_state = MachineState::Alive;
	}

	std::string machine_state_to_string() const {
		switch (this->machine_state) {
			case MachineState::Alive:
				return "alive";
			case MachineState::Failed:
				return "failed";
			case MachineState::Suspected:
				return "suspected";
			case MachineState::Leave:
				return "leave";
			default:
				return "Unknown";
		}	
	}

	void print(bool print_incarnation) const {
		std::cout << std::setw(left_indent) << std::fixed << this->machine_id << "|";
		std::cout << std::setw(left_indent) << std::fixed << this->machine_version_number << "|";
		std::cout << std::setw(left_indent) << std::fixed << this->heartbeat_counter << "|";
		if (print_incarnation) {
			std::cout << std::setw(left_indent) << std::fixed << this->incarnation_number << "|";
		}
		std::cout << std::setw(left_indent) << std::fixed << this->last_update_time << "|";
		std::cout << std::setw(left_indent) << std::fixed << this->machine_state_to_string() << "|";
		std::cout << std::endl;
	}

	bool last_update_time_above_threshold(unsigned long threshold, unsigned long current_time) const {
		if ((current_time - this->last_update_time) > threshold) {
#ifdef DEBUG
			std::cout << current_time << " - " << this->last_update_time << " = " << (current_time - this->last_update_time) << " > " << threshold << std::endl;
#endif
			return true;
		}
		return false;
	}

	bool check_if_node_suspected(unsigned long t_suspected, unsigned long current_time) {
		if (this->last_update_time_above_threshold(t_suspected, current_time)) {
			this->machine_state = MachineState::Suspected;
			return true;
		}
		return false;
	}

	bool check_if_node_failed(unsigned long t_suspected, unsigned long t_fail, unsigned long current_time) {
		if (this->last_update_time_above_threshold(t_fail + t_suspected, current_time)) {
			this->machine_state = MachineState::Failed;
			return true;
		}
		return false;
	}

	bool should_cleanup_node(unsigned long t_suspected, unsigned long t_fail, unsigned long t_cleanup, unsigned long current_time) {
		if (this->machine_state == MachineState::Leave) {
			return this->last_update_time_above_threshold(t_cleanup, current_time);
		}
#ifdef ASSERT
		assert(this->machine_state == MachineState::Failed);
#endif
		return this->last_update_time_above_threshold(t_cleanup + t_fail + t_suspected, current_time);
	}

	static uint32_t struct_serialized_size(bool gossip_with_suspect) {
		return sizeof(uint32_t) + // machine id 
			sizeof(uint32_t) + // machine version number
			sizeof(uint32_t) + // heartbeat counter  
			(gossip_with_suspect ? sizeof(uint32_t) + sizeof(uint32_t) : 0); // incarnation number and machine state
	}

	// Serializes the machine id, hearbeat counter, incarnation number, and machine state of this and writes serialized data into
	// buffer.
	// Precondition: The size of buffer must be >= value returned by this.struct_serialized_size()
	char* serialize(char* buffer, bool gossip_with_suspect) const {
		uint32_t network_machine_id = htonl(this->machine_id);
		memcpy(buffer, (char*)&network_machine_id, sizeof(uint32_t));
		buffer += sizeof(uint32_t);

		assert(this->machine_id != 0 && network_machine_id != 0);

		uint32_t network_machine_version_number = htonl(this->machine_version_number);
		memcpy(buffer, (char*)&network_machine_version_number, sizeof(uint32_t));
		buffer += sizeof(uint32_t);

		uint32_t network_machine_heartbeat = htonl(this->heartbeat_counter);
		memcpy(buffer, (char*)&network_machine_heartbeat, sizeof(uint32_t));
		buffer += sizeof(uint32_t);

		if (gossip_with_suspect) {
			uint32_t network_machine_incarnation_number = htonl(this->incarnation_number);
			memcpy(buffer, (char*)&network_machine_incarnation_number, sizeof(uint32_t));
			buffer += sizeof(uint32_t);

			uint32_t network_machine_state = htonl(this->machine_state);
			memcpy(buffer, (char*)&network_machine_state, sizeof(uint32_t));
			buffer += sizeof(uint32_t);
		}

		return buffer;
	}

	// Deserializes the machine id, hearbeat counter, incarnation number, and machine state from buffer. Updates member_information
	// with the deserialized values (returned via argument).
	// Precondition: The size of buffer must be >= value returned by this.struct_serialized_size()
	static const char* deserialize(const char* buffer, MemberInformation& member_information, bool gossip_with_suspect) {
		uint32_t network_machine_id;
		memcpy((char*)&network_machine_id, buffer, sizeof(uint32_t));
		member_information.machine_id = ntohl(network_machine_id);
		buffer += sizeof(uint32_t);

		assert(member_information.machine_id != 0 && network_machine_id != 0);

		uint32_t network_machine_version_number;
		memcpy((char*)&network_machine_version_number, buffer, sizeof(uint32_t));
		member_information.machine_version_number = ntohl(network_machine_version_number);
		buffer += sizeof(uint32_t);

		uint32_t network_machine_heartbeat;
		memcpy((char*)&network_machine_heartbeat, buffer, sizeof(uint32_t));
		member_information.heartbeat_counter = ntohl(network_machine_heartbeat);
		buffer += sizeof(uint32_t);

		if (gossip_with_suspect) {
			uint32_t network_machine_incarnation_number;
			memcpy((char*)&network_machine_incarnation_number, buffer, sizeof(uint32_t));
			member_information.incarnation_number = ntohl(network_machine_incarnation_number);
			buffer += sizeof(uint32_t);

			uint32_t network_machine_state;
			memcpy((char*)&network_machine_state, buffer, sizeof(uint32_t));
			member_information.machine_state = static_cast<MachineState>(ntohl(network_machine_state));
			buffer += sizeof(uint32_t);
		}

		return buffer;
	}

	bool has_failed() const {
		return this->machine_state == MachineState::Failed;
	}

	bool is_suspected() const {
		return this->machine_state == MachineState::Suspected;
	}

	bool is_alive() const {
		return this->machine_state == MachineState::Alive;
        }

	// Returns true if this machine id and machine version number match other machine id and version 
	bool is_same_member(uint32_t other_member_id, uint32_t other_member_version) {
		return (this->machine_id == other_member_id) && (this->machine_version_number == other_member_version);
	}

	// Returns true if the machine id and machine version number of this and other_memeber are the same
	bool is_same_member(const MemberInformation& other_member) {
		return is_same_member(other_member.machine_id, other_member.machine_version_number);
	}

	// Merges information of this and merge_source
	void merge_member_information(const MemberInformation& merge_source, unsigned long current_time) {
		// Handle case with leave status:
		// this has leave state: do nothing
		if (this->machine_state == MachineState::Leave) {
			return;
		}
		// merge_source has leave state: mark this as leave
		if (merge_source.machine_state == MachineState::Leave) {
			this->machine_state = MachineState::Leave;
			log_machine_left(this->machine_id, this->machine_version_number);

#ifdef LEAVE_PRINT
			std::cout << "LEAVE" << std::endl;
			print_header(not is_protocol_gossip());
			this->print(not is_protocol_gossip());
#endif

#ifdef DEBUG
			std::cout << current_time << " - " << this->machine_id << ":" << this->machine_version_number << ":LEAVE" << std::endl;
#endif
			return;
		}

		// Handle case with failed status:
		// this has delete state: do nothing
		if (this->machine_state == MachineState::Failed) {
			return;
		}
		// merge_source has failed state: mark this as failed
		if (merge_source.machine_state == MachineState::Failed) {
			this->machine_state = MachineState::Failed;
			failures += 1;
			log_machine_failed(this->machine_id, this->machine_version_number);

#ifdef FAILED_PRINT
			std::cout << current_time << " FAILED" << std::endl;
			print_header(not is_protocol_gossip());
			this->print(not is_protocol_gossip());
#endif

#ifdef DEBUG
			std::cout << current_time << " - " << this->machine_id << ":" << this->machine_version_number << ":FAILED" << std::endl;
#endif
			return;
		}

		// Handle case with suspect:
		// this and merge_source have suspect state: do nothing
		if (this->machine_state == MachineState::Suspected && merge_source.machine_state == MachineState::Suspected) {
			return;
		}
		// merge_source has suspect state (but this does not): if merge_source also has incarnation number >= this,
		// mark this as suspect; otherwise do nothing
		if (merge_source.machine_state == MachineState::Suspected) {
			if (merge_source.incarnation_number >= this->incarnation_number) {
				this->machine_state = MachineState::Suspected;
				log_machine_suspected(this->machine_id, this->machine_version_number);
				std::cout << "SUSPECTED:" << std::endl;
				print_header(not is_protocol_gossip());
				this->print(true);
#ifdef DEBUG
				std::cout << current_time << " - " << this->machine_id << ":" << this->machine_version_number << ":SUSPECTED" << std::endl;
#endif
			}
			return;
		}
		// this has suspect state (but merge_source does not): if merge source has higher incarnation number, mark this as alive
		if (this->machine_state == MachineState::Suspected) {
			if (merge_source.incarnation_number > this->incarnation_number) {
				this->machine_state = MachineState::Alive;
				this->incarnation_number = merge_source.incarnation_number;
				this->heartbeat_counter = merge_source.heartbeat_counter;
				this->last_update_time = current_time;
				log_machine_new_node(this->machine_id, this->machine_version_number);

#ifdef REJOIN_PRINT
				std::cout << "BACK ALIVE:" << std::endl;
				print_header(not is_protocol_gossip());
				this->print(true);
#endif

#ifdef DEBUG
				std::cout << current_time << " - " << this->machine_id << ":" << this->machine_version_number << ":BACK ALIVE" << std::endl;
#endif
			}
			return;
		}

		// Handle case with alive status:
		// this has alive state (and so does merge_source): update this heartbeat counter if merge_source's counter if higher
		if (this->machine_state == MachineState::Alive) {
			if (merge_source.heartbeat_counter > heartbeat_counter) {
				heartbeat_counter = merge_source.heartbeat_counter;
				last_update_time = current_time;
#ifdef DEBUG
				std::cout << current_time << " - " << this->machine_id << ":" << this->machine_version_number << ":NEW HEARTBEAT"
					<< this->heartbeat_counter << std::endl;
#endif
			}
		}
		return;
	}

	uint32_t get_machine_id() const {
		return this->machine_id;
	}

	uint32_t get_heartbeat_counter() const {
		return this->heartbeat_counter;
	}

	std::string get_machine_ip() const {
		return this->machine_ip;
	}

	uint32_t get_machine_version() const {
		return this->machine_version_number;
	}

	// Increments heartbeat counter and sets last_update_time to current_time
	void increment_heartbeat_counter(unsigned long current_time) {
		++this->heartbeat_counter;
		this->last_update_time = current_time;
	}

	// Increments incarnation number and sets machien state to alive
	void increment_incarnation_number() {
		++this->incarnation_number;
		this->machine_state = MachineState::Alive;
	}

	//set self state as leave
	void set_state_leave(){
		this->machine_state = MachineState :: Leave;
	}

	void set_state_suspected() {
		this->machine_state = MachineState::Suspected;
	}

};

class MembershipListManager {
private:
	std::vector<MemberInformation> membership_list; // stores information on all other nodes it knows about
	std::mutex mtx;					// mutex to guaard concurrent access to this class
	MemberInformation self_status;			// stores information on the machine this process is running on

	// The function updates membership entry of machine_id if machine_heartbeat_counter is greater than the heartbeat 
	// currently stored for that machine. If machine_id is not present in this->membership list, a new entry is created 
	// for machine_id
	// Caller: The function is called by this->merge_membership_list
	// Precondition: Requires that this->mtx already be held by the caller of this function
	void merge_membership_list_helper(const MemberInformation& machine_info, unsigned long current_time) {
		if (self_status.is_same_member(machine_info)) {
			if (machine_info.is_suspected()) {
				self_status.set_state_suspected();
			}
			return;
		}
		for (uint32_t i = 0; i < membership_list.size(); ++i) {
			if (membership_list[i].is_same_member(machine_info)) {
				membership_list[i].merge_member_information(machine_info, current_time);
				return;
			}
		}
		if (not machine_info.is_alive()) {
			return;
		}
		// machine_id is not present in this->membership_list. Treat it as a new node.
		uint32_t machine_id = machine_info.get_machine_id();
		uint32_t machine_version = machine_info.get_machine_version();
		log_machine_new_node(machine_id, machine_version);
#ifdef NEW_NODE_PRINT
		std::cout << "NEW NODE" << std::endl;
		print_header(not is_protocol_gossip());
		machine_info.print(not is_protocol_gossip());
#endif

#ifdef DEBUG
		std::cout << current_time << " - " << machine_id << ":" << machine_version << ":NEW ALIVE" << std::endl;
#endif
		membership_list.emplace_back(machine_id, get_machine_ip(machine_id), machine_version, machine_info, current_time);
	}

public:
	MembershipListManager() : self_status(-1, "", -1) {}

	MembershipListManager(uint32_t self_machine_id, uint32_t self_version_number) : 
		self_status(self_machine_id, get_machine_ip(self_machine_id), self_version_number) {}

	MembershipListManager(std::vector<int>& machine_ids, uint32_t self_machine_id, uint32_t self_version_number) : 
		self_status(self_machine_id, get_machine_ip(self_machine_id), self_version_number, 0, get_current_time()) {

			membership_list.reserve(machine_ids.size());
			unsigned long current_time = get_current_time();
			for (size_t i = 0; i < machine_ids.size(); ++i) {
				if (self_status.is_same_member(machine_ids[i], self_version_number)) {
					continue;
				} else {
					membership_list.emplace_back(machine_ids[i], get_machine_ip(machine_ids[i]), 0, 0, current_time);
				}
			}
		}

	void clear() {
		mtx.lock();
		this->membership_list.clear();
		this->self_status.set_state_leave();
		mtx.unlock();
	}

	void init_self(uint32_t self_machine_id, uint32_t self_version_number) {
		self_status.init(self_machine_id, get_machine_ip(self_machine_id), self_version_number);
	}

	const std::vector<MemberInformation>& get_membership_list() const {
		return this->membership_list;
	}

	const MemberInformation& self_get_status() const {
		return this->self_status;
	}

	const uint32_t get_self_id() const {
		return this->self_status.get_machine_id();
	}

	const uint32_t get_self_version() const {
		return this->self_status.get_machine_version();
	}

	// Set self state as Leave
	void self_set_state_leave() {
		this->self_status.set_state_leave();
	}

	void add_new_member(uint32_t machine_id, uint32_t machine_version_number) {
		mtx.lock();
		if (self_status.is_same_member(machine_id, machine_version_number)) {
			assert(false);
		}

		membership_list.emplace_back(machine_id, get_machine_ip(machine_id), machine_version_number, 0, get_current_time());
		log_machine_new_node(machine_id, machine_version_number);
#ifdef DEBUG
		std::cout << get_current_time() << " - " << machine_id << ":" << machine_version_number << ":NEW ALIVE" << std::endl;
#endif
		mtx.unlock();
	}

	// This function increments the heartbeat counter of machine id associated with current process and sets machine id's last updated 
	// time to current time.
	// Caller: The function will primarily be called by client to increment the heartbeat of its own machine at every gossip period.
	// The membership list should be gossiped out after the call to this function.
	// The function locks membership list internally
	void self_increment_heartbeat_counter() {
		mtx.lock();
		this->self_status.increment_heartbeat_counter(get_current_time());
		mtx.unlock();
	}

	// This function increments the incarnation number of machine id associated with current process and sets machine status to alive
	// Caller: The function will primarily be called by server if it detects the current machine has been suspected.
	// The function locks membership list internally
	void self_increment_incarnation_number() {
		mtx.lock();
		this->self_status.increment_incarnation_number();
		mtx.unlock();
	}

	// The function checks if the machine id associated with current process has suspected state.
	// Caller: The function will primarily be called by server. If the current machine is suspected, server will update the incarnation
	// number and update state to alive to it can start sending alive messages.
	// The function locks membership list internally
	bool self_is_suspected() {
		mtx.lock();
		bool return_val = this->self_status.is_suspected();
		mtx.unlock();
		return return_val;
	}

	// Used only for testing, the following function increments the heartbeat counter of member at index i in membership list.
	// The function locks membership list internally
	void increment_heartbeat_counter_testing_only(uint32_t i) {
		mtx.lock();
		this->membership_list[i].increment_heartbeat_counter(get_current_time());
		mtx.unlock();
	}

	// This function iterates over all machines in membership_list and performs checks if the last update heartbeat update time of 
	// machine is larger than:
	// 	1. t_suspected: then mark the machine as suspected and log it
	// 	2. t_suspected+t_fail: then mark the machine as failed and log it
	// 	2. t_suspected+t_fail+t_cleanup: remove the machine from the membership list
	// Caller: The function is called by client periodically to detect node failures
	// The function locks the membership list internally.
	void update_membership_list(unsigned long t_suspected, unsigned long t_fail, unsigned long t_cleanup, bool use_suspect_mechanism) {
		mtx.lock();
		if (not use_suspect_mechanism) {
			t_suspected = 0;
		}

		unsigned long current_time = get_current_time();
		for (int32_t i = membership_list.size() - 1; i >= 0; --i) {
			MemberInformation& member = membership_list[i];

			if (self_status.is_same_member(member)) {
				assert(false);
			}

			if (member.should_cleanup_node(t_suspected, t_fail, t_cleanup, current_time)) {
				log_machine_purged(member.get_machine_id(), member.get_machine_version());
#ifdef PURGED_PRINT
				std::cout << "PURGED" << std::endl;
				print_header(not is_protocol_gossip());
				member.print(not is_protocol_gossip());
#endif

#ifdef DEBUG
				std::cout << current_time << " - " << member.get_machine_id() << ":" << member.get_machine_version() << ":PURGED" << std::endl;
				std::cout << "threashold: " << t_suspected + t_fail + t_cleanup << std::endl;
#endif
				membership_list.erase(membership_list.begin() + i);
				continue;
			}
			if (member.has_failed()) {
				continue;
			}
			if (member.check_if_node_failed(t_suspected, t_fail, current_time)) {
				log_machine_failed(member.get_machine_id(), member.get_machine_version());
				failures += 1;
#ifdef FAILED_PRINT
				std::cout << current_time << " FAILED" << std::endl;
				print_header(not is_protocol_gossip());
				member.print(not is_protocol_gossip());
#endif

#ifdef DEBUG
				std::cout << current_time << " - " << member.get_machine_id() << ":" << member.get_machine_version() << ":FAILED" << std::endl;
				std::cout << "threashold: " << t_suspected + t_fail << std::endl;
#endif
				continue;
			}
			if (not use_suspect_mechanism || member.is_suspected()) {
				continue;
			}
			if (member.check_if_node_suspected(t_suspected, current_time)) {
				log_machine_suspected(member.get_machine_id(), member.get_machine_version());
				std::cout << "SUSPECTED" << std::endl;
				print_header(not is_protocol_gossip());
				member.print(not is_protocol_gossip());
#ifdef DEBUG
				std::cout << current_time << " - " << member.get_machine_id() << ":" << member.get_machine_version() << ":SUSPECTED" << std::endl;
				std::cout << "threashold: " << t_suspected << std::endl;
#endif
				continue;

			}
		}
		// No need to check this->self_status; that will updated every gossip period any ways so will always be alive

		mtx.unlock();
	}

	// The function updates membership entry of every machine in new_machine_heartbeats and adds new machines present in 
	// merge_membership_list to this->membership_list
	// Caller: The function is primarly called by server when it recieves a membership list from a client
	// The function locks the membership list internally.
	void merge_membership_list(const std::vector<MemberInformation>& merge_membership_list) {
		mtx.lock();
		unsigned long current_time = get_current_time();
#ifdef DEBUG
		std::cout << "merging at time: " << current_time << std::endl;
#endif
		for (const MemberInformation& machine_info : merge_membership_list) {
			merge_membership_list_helper(machine_info, current_time);
		}
		mtx.unlock();
	}

	// The function formats machine id and heartbeat of each non-failed machine in this->membership_list such that it can be
	// sent over the socket to other servers. 
	// The function returns a char pointer pointing to the serialized membership list. The memory is allocated on heap and as
	// such must be freed by caller. The function updates membership_list_length argument to indicate the size of serialized
	// membership list.
	// Caller: The function is primarily called by the client when it gossips its membership list
	// Postcondition: The char* returned is allocated on heap and must be freed by caller
	// The function locks membership list internally
	char* serialize_membership_list(uint32_t& membership_list_length, bool gossip_with_suspect) {
		mtx.lock();

		if (not this->self_status.is_alive()) {
			membership_list_length = 0;
			mtx.unlock();
			return NULL;
		}

		membership_list_length = MemberInformation::struct_serialized_size(gossip_with_suspect) * 
						(1 + membership_list.size()); // +1 for self status
		char* membership_list_as_str = new char[membership_list_length];
		char* buffer = membership_list_as_str;

		buffer = this->self_status.serialize(buffer, gossip_with_suspect);
		for (int32_t i = 0; i < membership_list.size(); ++i) {
			if (membership_list[i].has_failed()) {
				continue;
			}
			buffer = membership_list[i].serialize(buffer, gossip_with_suspect);
		}
		mtx.unlock();
		membership_list_length = (buffer - membership_list_as_str);

		return membership_list_as_str;
	}

	// The function deserializes the byte representation of membership list from another node and returns the deserialized information.
	// Caller: The function is primarily called by server when it recieves a membership list
	static std::vector<MemberInformation> deserialize_membership_list(const char* membership_list_as_str, uint32_t membership_list_length, 
			bool gossip_with_suspect) {
		std::vector<MemberInformation> machine_info;
		const char* buffer = membership_list_as_str;
		const char* buffer_end = membership_list_as_str + membership_list_length;
		while (buffer < buffer_end) {
			MemberInformation member;
			buffer = MemberInformation::deserialize(buffer, member, gossip_with_suspect);
			machine_info.push_back(member);
		}
		return machine_info;
	}

	void print_self(bool print_incarnation) {
		this->mtx.lock();
		std::cout << std::setw(left_indent) << std::fixed << "Machine Id" << "|";
		std::cout << std::setw(left_indent) << std::fixed << "Machine Version" << "|";
		std::cout << std::setw(left_indent) << std::fixed << "Heartbeat" << "|";
		if (print_incarnation) {
			std::cout << std::setw(left_indent) << std::fixed << "Incarnation" << "|";
		}
		std::cout << std::setw(left_indent) << std::fixed << "Update time" << "|";
		std::cout << std::setw(left_indent) << std::fixed << "Machine State" << "|";
		std::cout << std::endl;

		this->self_status.print(print_incarnation);
		this->mtx.unlock();
	}

	void print(bool print_incarnation) {
		this->mtx.lock();
		print_header(print_incarnation);

		this->self_status.print(print_incarnation);
		for (const MemberInformation& member_info : this->membership_list) {
			member_info.print(print_incarnation);
		}
		this->mtx.unlock();
	}

};

extern MembershipListManager membership_list_manager;
