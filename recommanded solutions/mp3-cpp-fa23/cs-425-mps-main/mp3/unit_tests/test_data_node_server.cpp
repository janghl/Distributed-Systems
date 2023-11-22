#include <fstream>
#include <iostream>
#include <sys/stat.h>

#include "../data_node/data_node_server.hpp"
#include "request.hpp"
#include "response.hpp"
#include "../../mp2/utils.hpp"
#include "../../mp2/membership_list_manager.hpp"

// ==== START OF EXTERN VARIABLES
std::string machine_id;
std::vector<std::string> machine_ips;
int grep_server_port = 4096;            // port on which server listens for incoming grep requests
pthread_t grep_server_thread;           // reference to thread running server on port grep_server_port

int introducer_server_port = 1024;      // port on which server listens for messages from new nodes
pthread_t introducer_server_thread;     // reference to thread running server on port introducer_server_port

int gossip_server_port = 2048;          // port on which server listens for heartbeats/membership lists from other nodes
pthread_t gossip_server_thread;         // reference to thread running server on port gossip_server_thread
pthread_t gossip_client_thread;         // reference to thread running client logic for gossip style failure detection

bool is_introducer_running;             // set to true if the current process is also running the introducer

bool end_session = false;               // set to true to get all threads to exit
bool exit_program = false;              // set to true to exit program

int message_drop_rate = 0;              // induced packet drop rate

MembershipListManager membership_list_manager;          // manages the membership list
ExtraInformationType* block_report = nullptr;
std::mutex protocol_mutex;                              // mutex taken by client/server whenever they access/update the generation count
// and membership_protocol
int generation_count;                                   // generation count of membership protocol. Every time a node
                                                        // changes the protocol, it updates the generation count and begins
                                                        // sending gossip messages with the new generation count and gossip protocol
                                                        // piggybacked on it
enum membership_protocol_options membership_protocol;   // reference to the type of failure detection protocol to run (can change
                                                        // mid execution)

// Variables to measure bandwdith
std::chrono::time_point<std::chrono::steady_clock> system_join_time;    // record time when machine joined system
uint64_t bytes_sent;                                    // total bytes the client gossiped to others
uint64_t bytes_received;                                // total bytes the server received

// Varibles to measure failures
uint64_t failures = 0;
uint64_t num_gossips = 0;
// ==== END OF EXTERN VARIABLES

uint32_t machine_id_int;
std::string file_system_root_dir = "test_block_report_server_fs_dir";

#define PORT 2202

int get_socket_with_server() {
	std::string machine_ip = get_machine_ip(machine_id_int);
        return setup_tcp_client_socket(machine_ip, PORT);
}

static const std::string sdfs_write_file_name = "sdfs_dummy_write_file.txt";
static char* sdfs_write_data = "abcdefghijklmnopqrstuvwxyz";
static const uint32_t sdfs_write_data_length = 26;
static const uint32_t write_update_counter = 31;
Request init_write_request() {
        WriteMetadata* write_meta = new WriteMetadata("dummy_write_file.txt");
        char* data = new char[sdfs_write_data_length];
        memcpy(data, sdfs_write_data, sdfs_write_data_length);
        return Request(OperationType::WRITE, sdfs_write_file_name, data, sdfs_write_data_length, write_update_counter, write_meta);
}
Request init_read_request() {
        ReadMetadata* read_meta = new ReadMetadata("dummy_read_file.txt");
        return Request(OperationType::READ, sdfs_write_file_name, read_meta);
}
Request init_delete_request() {
        return Request(OperationType::DELETE, sdfs_write_file_name, nullptr);
}

void test_write_request() {
	Request write_request = init_write_request();
	int socket = get_socket_with_server();
	send_request_over_socket(socket, write_request);
	Response* response = get_response_over_socket(socket);
	WriteSuccessReadResponse expected_response = WriteSuccessReadResponse(machine_id_int, OperationType::WRITE, sdfs_write_data_length, 
			sdfs_write_file_name, write_update_counter);
	assert(expected_response == *reinterpret_cast<WriteSuccessReadResponse*>(response));

	std::ifstream f(file_system_root_dir + "/" + sdfs_write_file_name);
	assert(f.good());
}

void test_read_request() {
        Request read_request = init_read_request();
        int socket = get_socket_with_server();
        send_request_over_socket(socket, read_request);
        Response* response = get_response_over_socket(socket);
        char* data = new char[sdfs_write_data_length];
        memcpy(data, sdfs_write_data, sdfs_write_data_length);
        WriteSuccessReadResponse expected_response = WriteSuccessReadResponse(machine_id_int, OperationType::READ, sdfs_write_data_length,
                        sdfs_write_file_name, write_update_counter, sdfs_write_data_length, data);
        assert(expected_response == *reinterpret_cast<WriteSuccessReadResponse*>(response));
}

void test_delete_request() {
        Request delete_request = init_delete_request();
        int socket = get_socket_with_server();
        send_request_over_socket(socket, delete_request);
        Response* response = get_response_over_socket(socket);
        Response expected_response = Response(machine_id_int, OperationType::DELETE, 0, sdfs_write_file_name);
        assert(expected_response == *response);

	std::ifstream f(file_system_root_dir + "/" + sdfs_write_file_name);
        assert(not f.good());
}

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Invalid program arguments. Run as " << argv[0] << " [id of current machine]" << std::endl;
		return 0;
	}
	mkdir(file_system_root_dir.c_str(), 0700);

	machine_id_int = std::stoi(argv[1]);

	BlockReport* block_report = new BlockReport(file_system_root_dir);
	pthread_t data_node_server_thread;
	launch_data_node_server(PORT, data_node_server_thread, block_report, machine_id_int);

	test_write_request();
	test_read_request();
	test_delete_request();

	return 0;
}
