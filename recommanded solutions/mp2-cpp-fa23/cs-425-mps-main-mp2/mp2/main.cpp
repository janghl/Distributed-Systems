#include <arpa/inet.h>

#include <cstring>
#include <chrono>
#include <sys/stat.h>
#include <iostream>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "utils.hpp"
#include "../mp1/client.hpp"
#include "mp1_wrapper.hpp"
#include "gossip_server.hpp"
#include "introducer.hpp"
#include "membership_list_manager.hpp"
#include "gossip_client.h"

// Note: for debugging, if you want to see the exact lines grep finds, compile with -D PRINT_ALL_LINES

// Ports and threads
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

// Variables for MP 1
std::string machine_id = ""; 		// id of machine running program
std::vector<std::string> machine_ips; 	// ip addresses of all other machines in cluster

int message_drop_rate = 0;		// induced packet drop rate

MembershipListManager membership_list_manager; 		// manages the membership list 

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
	std::cout << "cleanup handler invoked" << std::endl;
	end_session = true;

	membership_list_manager.clear(); 

	pthread_kill(gossip_client_thread, SIGUSR1);
	pthread_join(gossip_client_thread, NULL);

	pthread_kill(grep_server_thread, SIGUSR1);
	pthread_join(grep_server_thread, NULL);

	pthread_kill(gossip_server_thread, SIGUSR1);
	pthread_join(gossip_server_thread, NULL);

	if (is_introducer_running) {
		pthread_kill(introducer_server_thread, SIGUSR1);
		pthread_join(introducer_server_thread, NULL);
	}

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

void setup_gossip_threads(int machine_id, int machine_version, int introducer_machine_id) {
	system_join_time = std::chrono::steady_clock::now();
	bytes_sent = 0;
	bytes_received = 0;
	failures = 0;
	num_gossips = 0;

	end_session = false;
        // set self_status in membership list
        membership_list_manager.init_self(machine_id, machine_version);

	// launch all grep and gossip servers
        launch_grep_server(&grep_server_thread);
        launch_gossip_server(gossip_server_port, gossip_server_thread);

        // Run introducer if needed
        if (is_introducer_running) {
                launch_introducer_server(introducer_server_port, introducer_server_thread);
        } else {
                // If introducer is running on a different machine, contact it and use its membership to initialize ours
                init_self_using_introducer(machine_id, machine_version, introducer_machine_id, introducer_server_port);
		membership_list_manager.print(not is_protocol_gossip());
        }

        // launch gossip client
        launch_gossip_client(gossip_server_port, gossip_client_thread, introducer_machine_id, introducer_server_port, machine_id);
}

void setup_all_threads(int machine_id, int machine_version, int introducer_machine_id) {
	launch_grep_server(&grep_server_thread);
	setup_gossip_threads(machine_id, machine_version, introducer_machine_id);
}

std::string help = " -n [current machine id] -v [current machine version] -s [current machine name] "  
"-i [introducer machine number] -d [message drop rate] -o [other machine names, space separated] " 
"[-p [protocol - GOSSIP or GOSSIP_S - defaults to GOSSIP]]"
"[-I [introducer server port]-G [gossip server port] -R [grep server port] "  
"-V [1 to indicate process should run introducer server; otherwise 0]]";

void parse_cmd_args(int argc, char** argv, 
		std::string& machine_id, int& machine_id_int,
		int& machine_version, 
		std::string& machine_name,
		int& introducer_number,
		int& message_drop_rate,
		std::string& other_machine_names,
		int& introducer_port,
		int& gossip_server_port,
		int& grep_server_port,
		bool& run_introducer,
		enum membership_protocol_options& membership_protocol
		) {
	int opt;
	while ((opt = getopt(argc, argv, "n:v:s:i:d:o:I:G:R:V:p:")) != -1) {
		switch (opt) {
			case 'n': // current machine number
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
			case 'i': // introducer machine number
				{
					std::string s = std::string(optarg);
					introducer_number = convert_to_int(s, "Introducer number");
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
			case 'V': // should run introducer on current machine (introducer machine number must match current machine number)
				{
					std::string s = std::string(optarg);
					run_introducer = convert_to_int(s, "Should run introducer");
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

	generation_count = 0;
	membership_protocol = membership_protocol_options::GOSSIP;

	// setup signal handler for SIGINT
	struct sigaction act;
	memset(&act, '\0', sizeof(act));
	act.sa_handler = cleanup;
	if (sigaction(SIGINT, &act, NULL) < 0) {
		perror("failed to setup sigint handler");
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
	int introducer_machine_id = -1;
	std::string other_machine_names;
	bool should_run_introducer = true;

	parse_cmd_args(argc, argv,
			machine_id, machine_id_int,
			machine_version,
			machine_name,
			introducer_machine_id,
			message_drop_rate,
			other_machine_names,
			introducer_server_port,
			gossip_server_port,
			grep_server_port,
			should_run_introducer,
			membership_protocol
		      );
	is_introducer_running = should_run_introducer && (introducer_machine_id == machine_id_int);
	if (machine_id_int < 0 || machine_version < 0 || introducer_machine_id < 0) {
		std::cerr << "Must provide current machine id and version along with introducer machine id" << std::endl;
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
	setup_all_threads(machine_id_int, machine_version, introducer_machine_id);

	// launch client that takes input from stdin
	std::string command;
	while (not exit_program) {
		std::cout << "Enter grep command or failure detection protocol: ";
		std::getline(std::cin, command);

		if (command.substr(0, 4) == "grep") {
			send_grep_command_to_cluster(command, std::cout);
			continue;
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
			setup_all_threads(machine_id_int, ++machine_version, introducer_machine_id);
		} else if (command == "list_mem") {
			membership_list_manager.print(not is_protocol_gossip());
		} else if (command == "list_self") {
			membership_list_manager.print_self(not is_protocol_gossip());
		} else {
			std::cerr << "Unrecognized command: " << command << std::endl;
		}
	}
	return 0;
}
