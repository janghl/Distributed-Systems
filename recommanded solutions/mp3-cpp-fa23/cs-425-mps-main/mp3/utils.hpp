#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <cstdlib>
#include <unordered_set>
#include <functional>
#include <atomic>

#define FILE_NOT_FOUND_ERROR_CODE -5
#define FATAL_DATA_NODE_CRASH_QUORUM_IMPOSSIBLE -10 
#define REQUEST_DATA_BUFFER_SIZE (32 * 1024) // number of bytes of data serialized at a time
#define FOOTER 0xdeadbeef
#define MIN(a, b) ((a < b) ? a : b)

// Variables for MP 1
extern std::string machine_id;
extern std::vector<std::string> machine_ips;
extern int grep_server_port;

// Variables for MP 3
extern int data_node_server_port;
extern int name_node_server_port;
extern std::atomic<uint32_t> name_node_id;
extern uint32_t self_machine_number;

/*** The following functions come from mp2/utils
int setup_server_socket(int server_port, int socket_type, int protocol);
int setup_tcp_server_socket(int server_port);
int setup_udp_server_socket(int server_port);
int setup_client_socket(const std::string& machine_ip, int server_port);
int setup_udp_client_socket(const std::string& machine_ip, int server_port);
int setup_tcp_client_socket(const std::string& machine_ip, int server_port);

void process_machine_names(const std::vector<std::string>& machine_names);

void send_membership_list(int socket, const char* serialized_membership_list, uint32_t membership_list_size, uint32_t& hash_value,
                uint32_t& hash_a);
void send_membership_and_protocol(int socket, uint32_t self_id, uint32_t self_version, const char* serialized_membership_list, 
		uint32_t membership_list_size);
bool get_generation_count_and_protocol_from_socket(int socket, uint32_t& client_generation_count,
                membership_protocol_options& client_gossip_protocol);
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
***/

/***
// The following functions come from mp1/utils
ssize_t write_to_socket(int socket, const char* message, int message_size);
ssize_t read_from_socket(int socket, char* message, int message_size);
ssize_t read_from_socket(int socket, std::string& command);
void process_machine_names(int count, char** machine_names);
***/


bool compare_data(const char* d1, const char* d2, uint32_t len);

class Request;                  // forward decleration
using request_data_serializer = std::function<bool(std::unordered_set<int>, Request&, std::string, uint32_t)>;
bool send_request_over_socket(int socket, const Request& request, std::string print_statement_origin, request_data_serializer data_serializer);
bool get_request_over_socket(int socket, Request& request, std::string print_statement_origin);

void close_all_sockets(const std::unordered_set<int>& destination_sockets);
bool send_request_data_from_socket(std::unordered_set<int> destination_sockets, Request& request, std::string print_statement_origin, int32_t max_failed_sockets);
bool send_request_data_from_file(std::unordered_set<int> destination_sockets, Request& request, std::string print_statement_origin, int32_t max_failed_sockets);

class Response;                 // forward decleration
class WriteSuccessReadResponse; // forward decleration
class LSResponse;               // forward decleration
bool send_response_over_socket(int socket, const Response& response, std::string print_statement_origin);
bool send_response_over_socket(int socket, const WriteSuccessReadResponse& response, std::string print_statement_origin);
bool send_response_over_socket(int socket, const LSResponse& response, std::string print_statement_origin);
Response* get_response_over_socket(int socket, uint32_t& payload_size);
int get_tcp_socket_with_node(int node_id, int port);
int create_socket_to_datanode(uint32_t data_node_server_port, uint32_t datanode_id);
