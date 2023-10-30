#include <arpa/inet.h>

#include <cstring>
#include <iostream>
#include <ostream>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "utils.hpp"
#include "client.hpp"

// Sends command to server via the server_socket
ssize_t send_grep_command_to_socket(const std::string& command, int server_socket) {
        // write the size of command first
        size_t command_size = htonl(command.length());
        ssize_t nbytes = write_to_socket(server_socket, (char*)(&command_size), sizeof(size_t));
        if (nbytes != sizeof(size_t)) {
                return -1;
        }
        // send command 
        return write_to_socket(server_socket, command.c_str(), command.length());
}

// Sends command on every machine, collects the output from each live machine, and writes the output to output_stream
void send_grep_command_to_cluster(const std::string& command, std::ostream& output_stream) {
        std::vector<int> sockets;

	// Open a socket with machine and send each machine the grep command
        for (int i = 0; i < machine_ips.size(); ++i) {
                int socket = connect_with_server(machine_ips[i]);
                if (socket < 0) { // indicates server machine has crashed
                        continue;
                }
                sockets.push_back(socket);
                send_grep_command_to_socket(command, socket);
                shutdown(socket, SHUT_WR); // shutdown the writer side of socket to let server know we are done sending data
        }

	// Iterate over each server socket and print server reply to output_stream
	int total_matching_lines = 0;
        for (int socket : sockets) {
		// server reply has the form "vm[#]: [count of matching lines]" 
                char server_reply_buffer[SERVER_REPLY_BUFFER_SIZE]; 	// temporary buffer in which server reply is stored
		std::string matching_lines_count = ""; 			// the number of machine lines the server reported
		bool found_colon = false; 				// indicate if we have seen the prefix "vm[#]: " yet

                while (true) {
                        errno = 0;
                	memset(server_reply_buffer, 0, SERVER_REPLY_BUFFER_SIZE);
                        int nbytes = read(socket, server_reply_buffer, SERVER_REPLY_BUFFER_SIZE - 1);
                        if (nbytes > 0) {
				std::string reply_str = std::string(server_reply_buffer);
#ifndef PRINT_ALL_LINES
				if (not found_colon) {
					// check if reply contains ": " and if yes, the suffix (after ": ") holds the number of matching 
					// lines the socket found so extract it into matching_lines_count
					int index_of_colon = reply_str.find(": ");
					if (index_of_colon < reply_str.length()) {
						found_colon = true;
						matching_lines_count = reply_str.substr(index_of_colon + 2);
					}
				} else {
					matching_lines_count += reply_str;
				}
#endif
                                output_stream << server_reply_buffer;
                                continue;
                        }
                        if (errno == EINTR || errno == EAGAIN) {
                                continue;
                        }
                        break; // read returned error or EOF
                }
#ifndef PRINT_ALL_LINES
		total_matching_lines += matching_lines_count.length() > 0 ? stoi(matching_lines_count) : 0;
#endif
                close(socket);
        }
	output_stream << "total: " << total_matching_lines << std::endl;
	// send a special (non-prinable) byte to signal client is done writing all its output. This is extremely useful for testing
	output_stream << "\x08" << std::endl; 
}

