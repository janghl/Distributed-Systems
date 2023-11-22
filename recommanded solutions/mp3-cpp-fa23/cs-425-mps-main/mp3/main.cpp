#include <arpa/inet.h>

#include <cstring>
#include <chrono>
#include <sys/stat.h>
#include <poll.h>
#include <iostream>
#include <functional>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <atomic>

#include "utils.hpp"
#include "data_node_server.hpp"
#include "name_node_server.hpp"
#include "data_node_block_report_manager.hpp"
#include "data_node_client.hpp"
#include "name_node_identifier.hpp"
#include "multi_read.hpp"

#include "../mp1/client.hpp"

#include "../mp2/utils.hpp"
#include "../mp2/gossip_server.hpp"
#include "../mp2/introducer.hpp"
#include "../mp2/membership_list_manager.hpp"
#include "../mp2/gossip_client.h"
#include "../mp2/mp1_wrapper.hpp"

// Note: for debugging, if you want to see the exact lines grep finds, compile with -D PRINT_ALL_LINES

// Variables for MP 3
const int NUM_REPLICAS = 4;
const int X = 4; //majority of ACK in writing

int data_node_server_port = 8192;
pthread_t data_node_server_thread;

int name_node_server_port = 16384;
pthread_t name_node_server_thread;

int name_node_identifier_port = 1025;
pthread_t name_node_identifier_thread;

int multi_read_port = 1026;
pthread_t multi_read_thread;

BlockManager block_manager;

std::string filesystem_root = "sdfs_files";

std::atomic<uint32_t> name_node_id;
uint32_t self_machine_number;

// Variables for MP 1
std::string machine_id = ""; 		// id of machine running program
std::vector<std::string> machine_ips; 	// ip addresses of all other machines in cluster

// Variables for MP 2 
int grep_server_port = 4096; 		// port on which server listens for incoming grep requests
pthread_t grep_server_thread; 		// reference to thread running server on port grep_server_port

int introducer_server_port = 1024;	// port on which server listens for messages from new nodes
pthread_t introducer_server_thread; 	// reference to thread running server on port introducer_server_port

int gossip_server_port = 2048;		// port on which server listens for heartbeats/membership lists from other nodes
pthread_t gossip_server_thread; 	// reference to thread running server on port gossip_server_thread
pthread_t gossip_client_thread; 	// reference to thread running client logic for gossip style failure detection

bool is_introducer_running;		// set to true if the current process is also running the introducer

bool end_session = false; 		// set to true to get all threads to exit
bool exit_program = false;		// set to true to exit program

int message_drop_rate = 0;		// induced packet drop rate

MembershipListManager membership_list_manager; 		// manages the membership list 
ExtraInformationType* block_report = nullptr;		// manages metadata for files stored on node

// Variables to indicate the protocol currently in use
std::mutex protocol_mutex;				// mutex taken by client/server whenever they access/update the generation count
// and membership_protocol
int generation_count;					// generation count of membership protocol. Every time a node 
							// changes the protocol, it updates the generation count and begins
							// sending gossip messages with the new generation count and gossip protocol
							// piggybacked on it
enum membership_protocol_options membership_protocol;	// reference to the type of failure detection protocol to run (can change 
							// mid execution)

// Variables to measure bandwdith
std::chrono::time_point<std::chrono::steady_clock> system_join_time;	// record time when machine joined system
uint64_t bytes_sent;					// total bytes the client gossiped to others
uint64_t bytes_received;				// total bytes the server received

// Varibles to measure failures
uint64_t failures = 0;
uint64_t num_gossips = 0;

// Handler for SIGINT or LEAVE. If LEAVE calls this function, handler must be set 0. 
void cleanup(int handler) {
	if (handler == SIGINT) {
#ifdef FAILED_PRINT
		std::cout << "Failure time: " << get_current_time() << std::endl;
#endif
		abort();
	}
	end_session = true;

	membership_list_manager.clear(); 

	pthread_kill(gossip_client_thread, SIGUSR1);
	pthread_join(gossip_client_thread, NULL);

	pthread_kill(grep_server_thread, SIGUSR1);
	pthread_join(grep_server_thread, NULL);

	pthread_kill(gossip_server_thread, SIGUSR1);
	pthread_join(gossip_server_thread, NULL);

	pthread_kill(introducer_server_thread, SIGUSR1);
	pthread_join(introducer_server_thread, NULL);

	pthread_kill(name_node_identifier_thread, SIGUSR1);
	pthread_join(name_node_identifier_thread, NULL);

	pthread_kill(data_node_server_thread, SIGUSR1);
	pthread_join(data_node_server_thread, NULL);

	pthread_kill(name_node_server_thread, SIGUSR1);
	pthread_join(name_node_server_thread, NULL);

	pthread_kill(multi_read_thread, SIGUSR1);
	pthread_join(multi_read_thread, NULL);

	membership_list_manager.clear(); 
}

void signal_handler(int handler) {
}

void update_membership_protocol(membership_protocol_options new_protocol) {
	protocol_mutex.lock();
	++generation_count;
	membership_protocol = new_protocol;
	protocol_mutex.unlock();
	// NOTE: the change in protocol will take 4 gossip rounds to propogate through the system
}

int convert_to_int(const std::string& str, const std::string failure_prefix = "") {
	try {
		return std::stoi(str);
	} catch (...) {
		std::cerr << failure_prefix << " must be integer" << std::endl;
		exit(-1);
	}
}

// Splits str_to_split on space characters. Returns a vector all the split string
std::vector<std::string> split_string(const std::string& str_to_split) {
	std::vector<std::string> vec;
	std::stringstream ss(str_to_split);
	std::string substring;
	while (getline(ss, substring, ' ')) {
		vec.push_back(substring);
	}
	return vec;
}

// Checks if it can open connection with machine_ip (ip address) at port server_port. On success, returns a true; on failure, returns false.
bool machine_is_pingable(int machine_id, int server_port) {
        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
		fcntl(sock_fd, F_SETFL, O_NONBLOCK);

        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));

        server_address.sin_family = AF_INET;
        int port_num = server_port;
        server_address.sin_port = htons(port_num);
		std::string machine_ip = get_machine_ip(machine_id);

        if (inet_pton(AF_INET, machine_ip.c_str(), &server_address.sin_addr) <= 0) {
                std::cerr << "Invalid host ip address and/or port" << std::endl;
                return false;
        }

        int connectErr = connect(sock_fd, (struct sockaddr*)&server_address, sizeof(server_address));
        if (connectErr != 0) {
#ifdef DEBUG_MP3_MAIN
			std::cerr << "Connection with " << machine_ip << ":" << server_port << " failed" << std::endl;
			perror("machine_is_pingable: Connect error");
#endif			
			struct pollfd poll_event;
			poll_event.fd = sock_fd;
			poll_event.events = POLLIN;

			int poll_return = poll(&poll_event, 1, 100); // set timeout of 100ms

			if (poll_return <= 0) {
#ifdef DEBUG_MP3_MAIN
				std::cerr << "Poll with " << machine_ip << ":" << server_port << " failed or timedout" << std::endl;
				perror("Poll error");
#endif
				return false;
			}
        }

		close(sock_fd);

        return true;
}


void setup_gossip_threads(int machine_id, int machine_version, int& introducer_machine_id) {
	introducer_machine_id = -1;
	system_join_time = std::chrono::steady_clock::now();
	bytes_sent = 0;
	bytes_received = 0;
	failures = 0;
	num_gossips = 0;

	// set self_status in membership list
	using std::placeholders::_1;
	std::function<void(MemberInformation&)> data_node_failure_handler = std::bind(&BlockManager::handle_data_node_failure, &block_manager, _1);
	membership_list_manager.init_self(machine_id, machine_version, block_report, data_node_failure_handler);

	// launch gossip server
	launch_gossip_server(gossip_server_port, gossip_server_thread);

	// run introducer
	launch_introducer_server(introducer_server_port, introducer_server_thread);

	// we only have 10 machines (with id 1-10) running; ping everyone and check if anyone has the introducer running
	for (int i = 1; i <= 10; ++i) {
		if (machine_id == i) {
			continue;
		}
		if (machine_is_pingable(i, name_node_identifier_port)) {
			if (init_self_using_introducer(machine_id, machine_version, i, introducer_server_port)) {
				introducer_machine_id = i;
				break;
			}
		} else {
#ifdef DEBUG_MP3_MAIN
			std::cout << "MAIN: Machine " << i << " is not pingable" << std::endl;
#endif
		}
	}
	membership_list_manager.print(not is_protocol_gossip());

	// launch gossip client
	launch_gossip_client(gossip_server_port, gossip_client_thread, introducer_machine_id, introducer_server_port, machine_id);
}

void setup_dfs_threads(int machine_id, int introducer_machine_id) {
	// launch data node server
	launch_data_node_server(data_node_server_port, data_node_server_thread, block_report);
	// launch name node identifier
	launch_name_node_identifier_server(name_node_identifier_port, name_node_identifier_thread);
	if (introducer_machine_id != -1) {
		get_name_node_id(introducer_machine_id, name_node_identifier_port);
	} else {
		name_node_id.store(machine_id);
		block_manager.init_self();
	}
	std::cout << "MAIN: established name node id : " << name_node_id.load() << std::endl;
	// launch data node client
	launch_client(&membership_list_manager, name_node_server_port);
	// run name node if needed
	launch_name_node_server(name_node_server_port, name_node_server_thread, &block_manager);
}

void setup_all_threads(int machine_id, int machine_version) {
	end_session = false;
	self_machine_number = machine_id;
#ifdef BLOCKREPORT
	block_report = new BlockReport(filesystem_root);
#else
	block_report = new ExtraInformationType();
#endif
	
	launch_grep_server(&grep_server_thread);
	int introducer_machine_id;
	setup_gossip_threads(machine_id, machine_version, introducer_machine_id);

#ifdef DEBUG_MP3_MAIN
	std::cout << "MAIN: established introducer machine id : " << introducer_machine_id << std::endl;
#endif
	setup_dfs_threads(machine_id, introducer_machine_id);

	launch_multi_read_server(multi_read_port, multi_read_thread);
}

std::string help = " -i [current machine id]\n-v [current machine version]\n-s [current machine name]\n"  
"-d [message drop rate]\n-o [other machine names, space separated]\n" 
"[\n"
"\t-p [protocol - GOSSIP or GOSSIP_S - defaults to GOSSIP]\n"
"\t-I [introducer server port]\n\t-G [gossip server port]\n\t-R [grep server port]\n"  
"\t-D [data node server port]\n"
"\t-N [name node server port]\n"
"\t-T [name node identifier port]\n"
"\t-M [multi read port]\n"
"\n]";

void parse_cmd_args(int argc, char** argv, 
		std::string& machine_id, int& machine_id_int,
		int& machine_version, 
		std::string& machine_name,
		int& message_drop_rate,
		std::string& other_machine_names,
		int& name_node_port,
		int& introducer_port,
		int& gossip_server_port,
		int& grep_server_port,
		bool& run_name_node,
		enum membership_protocol_options& membership_protocol,
		int& data_node_server_port,
		int& name_node_identifier_port,
		int& multi_read_port
		) {
	int opt;
	while ((opt = getopt(argc, argv, "i:v:s:n:d:o:p:I:G:R:V:D:N:T:M:")) != -1) {
		switch (opt) {
			case 'i': // current machine number
				{
					machine_id = std::string(optarg);
					machine_id_int = convert_to_int(machine_id, "Machine number");
					break;
				}
			case 'v': // current machine version
				{
					std::string s = std::string(optarg);
					machine_version = convert_to_int(s, "Machine version");
					break;
				}
			case 's': // current machine name
				{
					machine_name = std::string(optarg);
					break;
				}
			case 'd': // message drop rate
				{
					std::string s = std::string(optarg);
					message_drop_rate = convert_to_int(s, "Message drop rate");
					break;
				}
			case 'o': // machine names (for all machines not including current one)
				{
					other_machine_names = std::string(optarg);
					break;
				}
			case 'p': // protocol to use
				{
					std::string protocol = std::string(optarg);
					if (protocol == "GOSSIP") {
						membership_protocol = membership_protocol_options::GOSSIP;
					} else if (protocol == "GOSSIP_S") {
                                                membership_protocol = membership_protocol_options::GOSSIP_S;
                                        }
					break;
				}
			case 'I': // introducer port number
				{
					std::string s = std::string(optarg);
					introducer_port = convert_to_int(s, "Introducer server port");
					break;
				}
			case 'G': // gossip server port
				{
					std::string s = std::string(optarg);
					gossip_server_port = convert_to_int(s, "Gossip server port");
					break;
				}
			case 'R': // grep  server port
				{
					std::string s = std::string(optarg);
					grep_server_port = convert_to_int(s, "Grep server port");
					break;
				}
			case 'D': // data node server port
				{
					std::string s = std::string(optarg);
                                        data_node_server_port = convert_to_int(s, "Data node server port");
                                        break;
				}
			case 'N': // name node server port
				{
					std::string s = std::string(optarg);
                                        name_node_server_port = convert_to_int(s, "Name node server port");
                                        break;
				}
			case 'V': // should run name node on current machine (name node machine number must match current machine number)
				{
					std::string s = std::string(optarg);
					run_name_node = convert_to_int(s, "Should run name node");
					break;
				}
			case 'T': // name node identifier port
				{
						std::string s = std::string(optarg);
						name_node_identifier_port = convert_to_int(s, "Name node identifier port");
						break;
				}

			case 'M': // multi read port
				{
						std::string s = std::string(optarg);
						multi_read_port = convert_to_int(s, "Multi read port");
						break;
				}
			default: /* '?' */
				std::cerr << "Invalid arguments; call program with " << argv[0] << " " << help << std::endl;
				exit(-1);
		}
	}
}

int main(int argc, char** argv) {
	mkdir("test", 0700);
	mkdir(filesystem_root.c_str(), 0700);

	generation_count = 0;
	membership_protocol = membership_protocol_options::GOSSIP;

	// setup signal handler for SIGINT & SIGPIPE
	struct sigaction act;
	memset(&act, '\0', sizeof(act));
	act.sa_handler = cleanup;
	if (sigaction(SIGINT, &act, NULL) < 0) {
		perror("failed to setup sigint handler");
		return 1;
	}
	struct sigaction act_pipe;
	memset(&act, '\0', sizeof(act_pipe));
	act_pipe.sa_handler = signal_handler;
	if (sigaction(SIGPIPE, &act_pipe, NULL) < 0) {
		perror("failed to setup sigpipe handler");
		return 1;
	}

	struct sigaction act_c;
	memset(&act_c, '\0', sizeof(act_c));
	act_c.sa_handler = signal_handler;
	if (sigaction(SIGUSR1, &act_c, NULL) < 0) {
			perror("failed to setup sigusr1 handler");
			return 1;
	}

	int machine_id_int = -1;
	int machine_version = -1;
	std::string machine_name = "";
	std::string other_machine_names;
	bool should_run_name_node = true;

	parse_cmd_args(argc, argv,
			machine_id, machine_id_int,
			machine_version,
			machine_name,
			message_drop_rate,
			other_machine_names,
			name_node_server_port,
			introducer_server_port,
			gossip_server_port,
			grep_server_port,
			should_run_name_node,
			membership_protocol,
			data_node_server_port,
			name_node_identifier_port,
			multi_read_port
		);
	if (machine_id_int < 0 || machine_version < 0) {
		std::cerr << "Must provide current machine id and version" << std::endl;
		std::cerr << "Call program with " << argv[0] << " " << help << std::endl;
		exit(-1);
	}
	std::vector<std::string> machine_names = split_string(other_machine_names);
	if (machine_name != "") {
		machine_names.push_back(machine_name);
	}

	// process ips of all machines running servers (this is used by MP 1 logic)
	process_machine_names(machine_names);

	// Start all the threads
	setup_all_threads(machine_id_int, machine_version);

	// launch client that takes input from stdin
	OperationType operation;
	std::string command;
	while (not exit_program) {
		std::cin.clear();
		std::cout << "Enter grep command or failure detection protocol: ";
		std::getline(std::cin, command);

		if (command.substr(0, 4) == "grep") {
			send_grep_command_to_cluster(command, std::cout);
		} else if (command == "disable suspicion") {
			update_membership_protocol(membership_protocol_options::GOSSIP);
		} else if (command == "enable suspicion") {
			update_membership_protocol(membership_protocol_options::GOSSIP_S);
		} else if (command == "leave") {
			// Print bandwidth
			std::chrono::time_point system_leave_time = std::chrono::steady_clock::now();
		        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(system_leave_time - system_join_time).count();
		        std::cout << "Out Bandwidth: " << (static_cast<double>(bytes_sent) / elapsed_seconds) << " bytes/second" << std::endl;
        		std::cout << "In Bandwidth: " << (static_cast<double>(bytes_received) / elapsed_seconds) << " bytes/second" << std::endl;
			std::cout << "Failures: " << failures << "/" << num_gossips << std::endl;
			std::cout << "False positive rate: " << (static_cast<double>(failures) / num_gossips) << " failures/gossip" << std::endl;

			send_leave_to_all(gossip_server_port);
			
			cleanup(0);
		} else if (command == "join") {
			setup_all_threads(machine_id_int, ++machine_version);
		} else if (command == "list_mem") {
			membership_list_manager.print(not is_protocol_gossip());
		} else if (command == "list_self") {
			membership_list_manager.print_self(not is_protocol_gossip());
		} else if (command == "store") {
			std::cout << block_report->get_file_names() << std::endl;
		} else if (command.substr(0, 9) == "multiread") {
			int32_t start_index = 10;
			int32_t sdfs_file_name_end = command.find(' ', start_index);
			std::string sdfs_file_name = command.substr(start_index, sdfs_file_name_end - start_index);

			int32_t local_file_name_end = command.find(' ', sdfs_file_name_end+1);
			std::string local_file_name = command.substr(sdfs_file_name_end+1, local_file_name_end - sdfs_file_name_end - 1);

#ifdef DEBUG_MP3_MAIN
			std::cout << "Multi read: local file : `" << local_file_name << "` sdfs file : `" << sdfs_file_name << "`" << std::endl;
#endif 
			std::vector<int> node_ids;
			int32_t prev_space_index = local_file_name_end;
			int32_t space_index = command.find(' ', prev_space_index+1);
			while (space_index >= 0) {
				std::string node_id_str = command.substr(prev_space_index+1, space_index - prev_space_index - 1);
#ifdef DEBUG_MP3_MAIN
			std::cout << "Multi read: node id: `" << node_id_str << "`"  << std::endl;
#endif 
				try {
					node_ids.push_back(std::stoi(node_id_str));
				} catch (...) {}
				prev_space_index = space_index;
				space_index = command.find(' ', prev_space_index+1);
			}
			std::string node_id_str = command.substr(prev_space_index+1);
#ifdef DEBUG_MP3_MAIN
			std::cout << "Multi read: node id: `" << node_id_str << "`"  << std::endl;
#endif 
			try {
				node_ids.push_back(std::stoi(node_id_str));
			} catch (...) {}

			send_multi_read(local_file_name, sdfs_file_name, node_ids, multi_read_port);
		} else if ((operation = is_sdfs_command(command)) != OperationType::INVALID) {
			handle_client_command(operation, command);
		} else {
			std::cerr << "Unrecognized command: " << command << std::endl;
		}
	}
	return 0;
}
