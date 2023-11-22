#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <unistd.h>
#include <string>
#include <iostream>

#include "../mp1/utils.hpp"
#include "../mp2/utils.hpp"
#include "../utils.hpp"
#include "name_node_identifier.hpp"

void launch_name_node_identifier_server(int port, pthread_t& thread) {
	int server_socket = setup_tcp_server_socket(port);
    pthread_create(&thread, NULL, run_name_node_identifier_server, reinterpret_cast<void*>(server_socket));
}

void send_name_node_id(int client_fd) {
    uint32_t network_name_node_id = htonl(name_node_id.load());
	if (write_to_socket(client_fd, (char*)&network_name_node_id, sizeof(uint32_t)) != sizeof(uint32_t)) {
        std::cout << "NAME_NODE_IDENTIFIER: Could not send name node id" << std::endl;
    }
	close(client_fd);
}

void* run_name_node_identifier_server(void* socket_ptr) {
	log_thread_started("name node identifier");
	int server_socket = static_cast<int>(reinterpret_cast<intptr_t>(socket_ptr));

        while(not end_session) {
                int new_fd = accept(server_socket, NULL, NULL);
                if (new_fd < 0) {
                    perror("Could not accept client");
                    break;
                }
                send_name_node_id(new_fd);
        }

        close(server_socket);
        return NULL;
}

void get_name_node_id(int introducer_machine_id, int name_node_identifier_port) {
	int socket = setup_tcp_client_socket(get_machine_ip(introducer_machine_id), name_node_identifier_port);
	if (socket < 0) {
		return;
	}
    uint32_t network_name_node_id;
	if (read_from_socket(socket, (char*)&network_name_node_id, sizeof(uint32_t)) != sizeof(uint32_t)) {
        std::cout << "NAME_NODE_IDENTIFIER: Could not read name node id" << std::endl;
    }
    name_node_id.store(ntohl(network_name_node_id));
    close(socket);
}

