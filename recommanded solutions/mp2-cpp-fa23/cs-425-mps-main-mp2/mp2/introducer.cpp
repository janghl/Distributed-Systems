#include <sys/socket.h>
#include <vector>
#include <unistd.h>
#include <string>
#include <iostream>

#include "utils.hpp"
#include "../mp1/utils.hpp"
#include "introducer.hpp"
#include "membership_list_manager.hpp"

void launch_introducer_server(int port, pthread_t& thread) {
	int server_socket = setup_tcp_server_socket(port);
        pthread_create(&thread, NULL, run_introducer_server, reinterpret_cast<void*>(server_socket));
}


void handle_new_node(int client_fd) {
	// Decode client message
        // client message has form: [client machine id - type uint32_t] [client machine version - type uint32_t]
        uint32_t client_machine_id = 0;
        if (read_from_socket(client_fd, (char*)&client_machine_id, sizeof(uint32_t)) < sizeof(uint32_t)) {
		close(client_fd);
#ifdef DEBUG
		std::cerr << "Client did not send machine id" << std::endl;
#endif
	}
        client_machine_id = (uint32_t)ntohl(client_machine_id);

        uint32_t client_machine_version = 0;
        if (read_from_socket(client_fd, (char*)&client_machine_version, sizeof(uint32_t)) < sizeof(uint32_t)) {
		close(client_fd);
#ifdef DEBUG
		std::cerr << "Client did not send machine version" << std::endl;
#endif
	}
        client_machine_version = (uint32_t)ntohl(client_machine_version);

	// Add client to membership list
	membership_list_manager.add_new_member(client_machine_id, client_machine_version);

#ifdef DEBUG
	std::cout << "Added node to membership list: " << std::endl;
	membership_list_manager.print(false);
#endif

	// Serialize local membership list and send it along with generation count, and gossip protocol to client
	uint32_t payload_size;
	char* payload = membership_list_manager.serialize_membership_list(payload_size, not is_protocol_gossip());
	if (payload != NULL) {
		send_membership_and_protocol(client_fd, membership_list_manager.get_self_id(), membership_list_manager.get_self_version(), 
				payload, payload_size);
		delete[] payload;
	}

	close(client_fd);
}

void* run_introducer_server(void* socket_ptr) {
	log_thread_started("introducer");
	int server_socket = static_cast<int>(reinterpret_cast<intptr_t>(socket_ptr));

        while(not end_session) {
                int new_fd = accept(server_socket, NULL, NULL);
                if (new_fd < 0) {
                        perror("Could not accept client");
                        break;
                }
                handle_new_node(new_fd);
        }

        close(server_socket);
        return NULL;
}

void send_self_information_to_introducer(int socket, uint32_t machine_id, uint32_t machine_version) {
	uint32_t network_machine_id = htonl(machine_id);
	write_to_socket(socket, (char*)&network_machine_id, sizeof(uint32_t));
	uint32_t network_machine_version = htonl(machine_version);
	write_to_socket(socket, (char*)&network_machine_version, sizeof(uint32_t));
	shutdown(socket, SHUT_WR);
}

void get_introducer_information(int socket) {
	get_gossip_message(socket, false);
	close(socket);
}

void init_self_using_introducer(uint32_t machine_id, uint32_t machine_version, int introducer_machine_id, int introducer_port) {
	int socket = setup_tcp_client_socket(get_machine_ip(introducer_machine_id), introducer_port);
	if (socket < 0) {
		return;
	}
	send_self_information_to_introducer(socket, machine_id, machine_version);
	get_introducer_information(socket);
}

