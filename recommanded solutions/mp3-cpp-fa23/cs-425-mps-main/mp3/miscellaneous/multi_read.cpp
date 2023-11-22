#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <unistd.h>
#include <string>
#include <iostream>

#include "../mp1/utils.hpp"
#include "../mp2/utils.hpp"
#include "utils.hpp"
#include "multi_read.hpp"
#include "data_node_client.hpp"

void launch_multi_read_server(int port, pthread_t& thread) {
	int server_socket = setup_tcp_server_socket(port);
    pthread_create(&thread, NULL, run_multi_read_server, reinterpret_cast<void*>(server_socket));
}

void handle_read_request(int client_fd) {
    uint32_t request_length;
	if (read_from_socket(client_fd, (char*)&request_length, sizeof(uint32_t)) != sizeof(uint32_t)) {
        std::cout << "MULTI_READ: Could not read request length" << std::endl;
        close(client_fd);
        return;
    }
    request_length = ntohl(request_length);

    char* request = new char[request_length];
    if (read_from_socket(client_fd, request, request_length) != request_length) {
        std::cout << "MULTI_READ: Could not read request" << std::endl;
        delete[] request;
        close(client_fd);
        return;
    }
    handle_client_command(OperationType::READ, std::string(request));
    delete[] request;
	close(client_fd);
}

void* run_multi_read_server(void* socket_ptr) {
	log_thread_started("multi read");
	int server_socket = static_cast<int>(reinterpret_cast<intptr_t>(socket_ptr));

        while(not end_session) {
                int new_fd = accept(server_socket, NULL, NULL);
                if (new_fd < 0) {
                    perror("Could not accept client");
                    break;
                }
                handle_read_request(new_fd);
        }

        close(server_socket);
        return NULL;
}

void send_multi_read(std::string local_file, std::string sdfs_file, const std::vector<int>& node_ids, int multi_read_port) {
    std::string request = "get " +  sdfs_file + " " + local_file;
    uint32_t network_request_length = htonl(request.length());
    for (int node_id : node_ids) {
        int socket = setup_tcp_client_socket(get_machine_ip(node_id), multi_read_port); 
        if (socket < 0) {
            std::cout << "MULTI_READ: Could not send read request to node " << node_id << std::endl;
            continue;
        }

        if (write_to_socket(socket, (char*)&network_request_length, sizeof(uint32_t)) !=  sizeof(uint32_t)) {
            std::cout << "MULTI_READ: Could not send read request to node " << node_id << std::endl;
            close(socket);
            continue;
        }
        if (write_to_socket(socket, request.c_str(), request.length()) != request.length()) {
            std::cout << "MULTI_READ: Could not send read request to node " << node_id << std::endl;
            close(socket);
            continue;
        }
        close(socket);   
    }
}

