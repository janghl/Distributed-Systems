#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <cstdlib>

#define MAX_CLIENTS 100
#define SERVER_REPLY_BUFFER_SIZE 512

class MemberInformation; // forward declaration

extern bool end_session;

// Variables for MP 1
extern std::string machine_id;
extern std::vector<std::string> machine_ips;
extern int grep_server_port;

extern int message_drop_rate; // in terms of % (ex. message_drop_rate = 5, for 5% drop rate)

// Variables to indicate the protocol currently in use
enum membership_protocol_options {GOSSIP, GOSSIP_S};
extern std::mutex protocol_mutex;
extern int generation_count;
extern membership_protocol_options membership_protocol;

// Variables to measure bandwdith
extern uint64_t bytes_sent;
extern uint64_t bytes_received;

// Variables to measure number of failures
extern uint64_t failures;
extern uint64_t num_gossips;

unsigned long get_current_time();

int setup_server_socket(int server_port, int socket_type, int protocol);
int setup_tcp_server_socket(int server_port);
int setup_udp_server_socket(int server_port);
int setup_client_socket(const std::string& machine_ip, int server_port);
int setup_udp_client_socket(const std::string& machine_ip, int server_port);
int setup_tcp_client_socket(const std::string& machine_ip, int server_port);

/***
// The following functions come from mp1/utils
ssize_t write_to_socket(int socket, const char* message, int message_size);
ssize_t read_from_socket(int socket, char* message, int message_size);
ssize_t read_from_socket(int socket, std::string& command);
void process_machine_names(int count, char** machine_names);
***/
void process_machine_names(const std::vector<std::string>& machine_names);

void send_membership_list(int socket, const char* serialized_membership_list, uint32_t membership_list_size, uint32_t& hash_value,
                uint32_t& hash_a);
void send_membership_and_protocol(int socket, uint32_t self_id, uint32_t self_version, const char* serialized_membership_list, 
		uint32_t membership_list_size);
//bool get_membership_list_from_socket(int socket, std::vector<MemberInformation>& client_membership_list, bool deserialize_incarnation);
bool get_generation_count_and_protocol_from_socket(int socket, uint32_t& client_generation_count,
                membership_protocol_options& client_gossip_protocol);
//bool get_membership_and_protocol(int socket, uint32_t& client_generation_count, membership_protocol_options& client_gossip_protocol,
//                                        std::vector<MemberInformation>& client_membership_list);
void get_gossip_message(int socket, bool drop_message);

std::string get_machine_ip(uint32_t machine_id);
std::string get_machine_name(uint32_t machine_id);

void log_info(char* message, std::string file_path);
void log_error(char* message, std::string file_path);
void log_warning(char* message, std::string file_path);
void log_debug(char* message, std::string file_path);
void log_machine_suspected(uint32_t machine_id, uint32_t machine_version);
void log_machine_failed(uint32_t machine_id, uint32_t machine_version);
void log_machine_left(uint32_t machine_id, uint32_t machine_version);
void log_machine_purged(uint32_t machine_id, uint32_t machine_version);
void log_machine_new_node(uint32_t machine_id, uint32_t machine_version);
void log_thread_started(std::string thread_info);

bool is_protocol_gossip();
void hash_message(const char* message, uint32_t message_length, uint32_t& hash, uint32_t& a);
