#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#include "data_node_client.hpp"
#include "data_node_server.hpp"
#include "name_node_server.hpp"
#include "request.hpp"
#include "response.hpp"

#include "../../mp2/utils.hpp"
#include "../mp2/introducer.hpp"
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
std::string local_file_data = "hello world\nI am a file that data node test will upload to our SDFS";
std::string local_file_name = "test_write_local_file.txt";
std::string local_file_copy_name = "test_read_into_local_file.txt";
std::string sdfs_file_name = "dummy_write_file.txt";

int name_node_server_socket;

#define NAME_NODE_PORT 1101
#define DATA_NODE_PORT 2202

int data_node_server_port = DATA_NODE_PORT;
const int NUM_REPLICAS = 2;
const int X = 1; //majority of ACK in writing

std::string get_machine_name(int machine_number) {
        std::string machine_number_str = std::to_string(machine_number);
        if (machine_number_str.length() < 2) {
                machine_number_str = "0" + machine_number_str;
        } else if (machine_number_str.length() > 2) {
                std::cerr << "Invalid machine number; machine number has more than 2 digits" << std::endl;
                exit(-1);
        }
        return "fa23-cs425-72" + machine_number_str + ".cs.illinois.edu";
}

void* execute_shell_command(void* command) {
    system((char*)command);
    return nullptr;
}

pthread_t launch_data_node_server_on_node(int node_id, int name_node_id) {
    std::string node_name = get_machine_name(node_id);
    std::string ssh_base = "ssh " + node_name + " cs-425/mp3/sdfs -i " + std::to_string(node_id) + " -v 0 -s " + node_name + " -n " + std::to_string(name_node_id) + 
							" -D " + std::to_string(DATA_NODE_PORT) + " -N " + std::to_string(NAME_NODE_PORT);
    pthread_t thread;
	char* command = new char[ssh_base.length() + 1];
	command[ssh_base.length()] = '\0';
	memcpy(command, ssh_base.c_str(), ssh_base.length());
    pthread_create(&thread, NULL, execute_shell_command, command);
    return thread;
}

void terminate_process_on_node(int node_id, pthread_t thread) {
	std::string machine_name = get_machine_name(node_id);
	std::string machine_ssh = machine_name;
	std::string kill_command = "\"ps -ef | grep sdfs | grep -v test | awk '{print $2}' | xargs kill -9\"";
	std::string terminate_process_command = "ssh " + machine_ssh + " " + kill_command;
	system(terminate_process_command.c_str());

	pthread_join(thread, NULL);
}

void test_read_repair_on_read_error() {
    // setup data node server on current machine
    int data_node_server_socket = setup_tcp_server_socket(DATA_NODE_PORT);

    // ==== WRITE

	// act as data node client and send write request to name node
	handle_client_command(OperationType::WRITE, "put " + local_file_name + " " + sdfs_file_name);

	// act as data node server and check that name node sent a write request
	{
		Request request;
		int name_node_fd = accept(data_node_server_socket, NULL, NULL);
		get_request_over_socket(name_node_fd, request, "TEST: ");
		char* data = new char[local_file_data.length()];
			memcpy(data, local_file_data.c_str(), local_file_data.length());
		Request expected_request = Request(OperationType::WRITE, sdfs_file_name, data, local_file_data.length(), 1, nullptr);
		assert(expected_request == request);

		{
			std::ifstream f(file_system_root_dir + "/" + sdfs_file_name);
			assert(!f.good());
		}

		close(name_node_fd); // do not send a response to the name node server and do not process the request

	}

	sleep(2); // let the write on other node complete

    // ==== READ

    // act as data node client and send read request to name node
	handle_client_command(OperationType::READ, "get " + local_file_copy_name + " " + sdfs_file_name);

    // act as data node server and check name node sent a read request
	{
		Request request;
		int name_node_fd = accept(data_node_server_socket, NULL, NULL);
		get_request_over_socket(name_node_fd, request, "TEST: ");
		Request expected_request = Request(OperationType::READ, sdfs_file_name, nullptr);
		assert(expected_request == request);

		// act as data node server and reply to read request
		sleep(2); // let the read on other node complete
		handle_data_node_read(name_node_fd, request, block_report);
	}

    // ==== READ REPAIR

    // act as data node server and check name node sent a read repair request
	{
		Request request;
		int name_node_fd = accept(data_node_server_socket, NULL, NULL);
		get_request_over_socket(name_node_fd, request, "TEST: ");
		char* data = new char[local_file_data.length()];
			memcpy(data, local_file_data.c_str(), local_file_data.length());
		Request expected_request = Request(OperationType::WRITE, sdfs_file_name, data, local_file_data.length(), 1, nullptr);
		assert(expected_request == request);

		{
				std::ifstream f(local_file_copy_name);
				assert(f.good());
		}

		// act as data node server and reply to write request
		handle_data_node_write(name_node_fd, request, block_report);
	}

    {
		std::ifstream f(file_system_root_dir + "/" + sdfs_file_name);
		assert(f.good());
	}

	sleep(3);

	std::cout << "TESTING COMPLETE" << std::endl;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Invalid program arguments. Run as " << argv[0] << " [id of current machine]" << std::endl;
		return 0;
	}
	mkdir(file_system_root_dir.c_str(), 0700);
	std::remove(local_file_copy_name.c_str());

    // setup local file
	std::ofstream file(local_file_name, std::ofstream::out | std::ofstream::trunc);
	if (!file) {
		std::cout << "Could not open file for writing" << std::endl;
		std::perror("ofsteam open failed");
		return -1;
	}
	file.write(local_file_data.c_str(), local_file_data.length());
	file.close();

    // setup self membership list
	machine_id_int = std::stoi(argv[1]);
	block_report = new BlockReport(file_system_root_dir);
	membership_list_manager.init_self(machine_id_int, 0, block_report);
    
    BlockManager block_manager;

    // launch name node and data node client on current machine and launch data node server on the next machine
    pthread_t name_node_server_thread;
    launch_name_node_server(NAME_NODE_PORT, name_node_server_thread, &block_manager);
	pthread_t introducer_server_thread;
	launch_introducer_server(introducer_server_port, introducer_server_thread);
	launch_client(&membership_list_manager, NAME_NODE_PORT);
    pthread_t other_machine_thread = launch_data_node_server_on_node((machine_id_int + 1) % 10, machine_id_int);

	while (membership_list_manager.get_membership_list().size() == 0) {
		sleep(2); // wait for all servers and the other node to come up
	}

	// test_read_repair_on_read_error();

	terminate_process_on_node((machine_id_int + 1) % 10, other_machine_thread);

	return 0;
}
