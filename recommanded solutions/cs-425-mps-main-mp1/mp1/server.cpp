#include "server.hpp"
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string>
#include <signal.h>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include <iostream>

#include "utils.hpp"

// Executes grep command and return a string representing output
std::string execute_grep_command(std::string & command){
	std::string full_command = command + " test/*.log"; 
#ifndef PRINT_ALL_LINES
	full_command += " -ch | awk 'BEGIN { sum=0 } { sum+=$1 } END {print sum }'";
#endif 
	std::array<char, 128> buffer; 	// temporary buffer to store output of fgets
	std::string result = ""; 	// stores all the output from grep; this is return
#ifndef PRINT_ALL_LINES
	result += "vm" + machine_id + ": ";
#endif

	FILE* pipe = popen(full_command.c_str(), "r");
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}

	while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
		result += buffer.data();
	}

	pclose(pipe);
	return result;
}

// Runs grep command sent by client, sends client responds and closes the socket with client. client_socket should be socket on which
// to interact with the client.
void handle_client(int client_socket) {
	// client packet has the following format: [size/length of grep command - type size_t][grep command - type char*]
	size_t command_length;
	read_from_socket(client_socket, (char*)&command_length, sizeof(size_t));
	command_length = (size_t)ntohl(command_length);

	std::string command;
	ssize_t nbytes = read_from_socket(client_socket, command);
	if (nbytes < 1 || command_length != nbytes){
		close(client_socket);
		return;
	}
#ifdef TESTING
	// special behaviour for testing; allows testing for situations when server crashes or doesn't send any response
	if (command == "grep TEST_SERVER_EXIT_UPON_CONNECTION") {
		close(client_socket);
		return;
	}
	if (command == "grep TEST_SERVER_CRASH_UPON_CONNECTION") {
		raise(SIGKILL);
		return;
	}
#endif

	std::string result = execute_grep_command(command);
	write_to_socket(client_socket, result.c_str(), result.size());
	close(client_socket);

}

// Listens for client requests on socket fd represented by server_socket_cast. Handles each client sequentially. Exits and closes socket
// when end_session flag is set to true. 
void* run_server(void* server_socket_cast) {
	int server_socket = static_cast<int>(reinterpret_cast<intptr_t>(server_socket_cast));

	while(not end_session) {
		int new_fd = accept(server_socket, NULL, NULL);	
		if (new_fd < 0) {
			perror("Could not accept client");
			break;
		}
		handle_client(new_fd);
	}

	close(server_socket);
	return NULL;
}
