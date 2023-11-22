#include <vector>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <poll.h>
#include <iostream>

#include "utils.hpp"
#include "../mp2/utils.hpp"
#include "name_node_server.hpp"
#include "operation_type.hpp"
#include "request.hpp"
#include "name_node_block_report_manager.hpp"
#include "../mp2/membership_list_manager.hpp"

BlockManager* block_manager_global;
bool name_node_active = false;

void launch_name_node_server(int port, pthread_t& thread, BlockManager* block_manager) {
	int server_socket = setup_tcp_server_socket(port);
	block_manager_global = block_manager;
	pthread_create(&thread, NULL, run_name_node_server, reinterpret_cast<void*>(server_socket));
}

void handle_name_node_client(int client_fd) {
	if (!block_manager_global->inited()) {
		close(client_fd);
		return;
	}
	Request client_request;
#ifdef DEBUG_MP3_NAME_NODE_SERVER
	std::cout << "NAME_NODE_SERVER: " << "Getting client request..." << std::endl;
#endif
	if (!get_request_over_socket(client_fd, client_request, "NAME_NODE_SERVER: ")) {
		std::cout << "NAME_NODE_SERVER: " << "Could not parse client request" << std::endl;
		return;
	}
#ifdef DEBUG_MP3_NAME_NODE_SERVER
	std::cout << "NAME_NODE_SERVER: " << "Got client request; adding it to the queues" << std::endl;
#endif
	block_manager_global->distribute_request(client_fd, client_request);
}

void* run_name_node_server(void* socket_ptr) {
	log_thread_started("name node server");
	int server_socket = static_cast<int>(reinterpret_cast<intptr_t>(socket_ptr));
	fcntl(server_socket, F_SETFL, O_NONBLOCK);

	while (not end_session) {
		struct pollfd poll_event;
		poll_event.fd = server_socket;
		poll_event.events = POLLIN;

		int poll_return = poll(&poll_event, 1, 1000); // timeout of 1000ms = 1sec

		if (poll_return < 0) {
			perror("NameNode: poll error");
			continue;
		} else if (poll_return == 0) { // poll timed out
#ifdef DEBUG_MP3_NAME_NODE_SERVER
			std::cout << "poll timed out" << std::endl;
#endif
			uint32_t current_name_node_id = name_node_id.load();
			if (current_name_node_id == self_machine_number && name_node_active) { // we are still the leader :)
#ifdef DEBUG_MP3_NAME_NODE_SERVER
				std::cout << "we are the leader" << std::endl;
#endif
				continue;
			}
			name_node_active = false; // we are not the leader

			if (current_name_node_id != self_machine_number && membership_list_manager.has_not_failed(current_name_node_id)) { // the leader is alive/suspected :)
#ifdef DEBUG_MP3_NAME_NODE_SERVER
				std::cout << "we are not the leader; but the leader is alive/suspected" << std::endl;
#endif
				continue;
			}
			// the leader has died. Node with the the lowest node id is our next leader
			if (self_machine_number != membership_list_manager.get_lowest_node_id()) { // we are not to be next leader; there is a node with lower node id that us
				name_node_id.store(membership_list_manager.get_lowest_node_id());
				std::cout << "Changed leader: " << name_node_id.load() << std::endl;
#ifdef DEBUG_MP3_NAME_NODE_SERVER
				std::cout << "leader is dead; but we are not to be leader" << std::endl;
#endif
				continue;
			}
			// we are the new leader!!
			std::cout << "we are the new leader" << std::endl;
			block_manager_global->init_self();
			std::cout << "I AM NOW THE LEADER HAHAHAH!!!!!" << std::endl;
			name_node_id.store(self_machine_number);
			name_node_active = true;
		} else {
			int new_fd = accept(server_socket, NULL, NULL);
			if (new_fd < 0) {
				perror("NameNode: Could not accept client");
				continue;
			}
			handle_name_node_client(new_fd);
		}
	}

	close(server_socket);
	return NULL;
}