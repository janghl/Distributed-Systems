#include <arpa/inet.h>

#include <cstring>
#include <iostream>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "utils.hpp"
#include "client.hpp"
#include "server.hpp"

// Note: for debugging, if you want to see the exact lines grep finds, compile with -D PRINT_ALL_LINES

int server_port = 4096; 		// port on which server listens for incoming client requests
bool end_session = false; 		// set to true to indicate program to exit
std::string machine_id = ""; 		// id of machine running program
std::vector<std::string> machine_ips; 	// ip addresses of all other machines in cluster
pthread_t server_thread; 		// reference to thread running server

// Handler for SIGINT
void cleanup(int handler) {
	end_session = true;
	pthread_join(server_thread, NULL);
}

int main(int argv, char** argc) {
	if (argv < 3) {
		std::cerr << "Invalid arguments; call program with " << argc[0] << " [current machine number] [machine 1 name] ... " << std::endl;
		return -1;
	}

	// setup singal handler for SIGINT
	struct sigaction act;
	memset(&act, '\0', sizeof(act));
	act.sa_handler = cleanup;
	if (sigaction(SIGINT, &act, NULL) < 0) {
		perror("failed to setup sigint handler");
		return 1;
	}

	// extract id of machine the process is run on and the ips of all machines running servers
	machine_id = std::string(argc[1]);
	process_machine_names(argv - 2, argc + 2); // argc[i] = name of machine running server for all i >= 2

	// launch server
	int server_socket = setup_server_socket();
	pthread_create(&server_thread, NULL, run_server, reinterpret_cast<void*>(server_socket));

	// launch client
	std::string grep_command;
	while (not end_session) {
		std::cout << "Enter grep command: ";
		std::getline(std::cin, grep_command);
		
		send_grep_command_to_cluster(grep_command, std::cout);
	}

	return 0;
}
