#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>

#include "utils.hpp"
#include "../mp1/utils.hpp"
#include "membership_list_manager.hpp"
#include "gossip_server.hpp"

void launch_gossip_server(int port, pthread_t& thread) {
#ifdef TCP
	int server_socket = setup_tcp_server_socket(port);
#else
	int server_socket = setup_udp_server_socket(port);
#endif
	pthread_create(&thread, NULL, run_gossip_server, reinterpret_cast<void*>(server_socket));
}

void handle_gossip_client(int client_fd) {
	bool drop_message = (rand() % 100 < message_drop_rate); // 'drop' message by simply not processing it
	get_gossip_message(client_fd, drop_message);
}

void* run_gossip_server(void* socket_ptr) {
	log_thread_started("gossip server");
	int server_socket = static_cast<int>(reinterpret_cast<intptr_t>(socket_ptr));

	while(not end_session) {
#ifdef TCP
		int new_fd = accept(server_socket, NULL, NULL);
		if (new_fd < 0) {
			perror("Could not accept client");
			break;
		}
		handle_gossip_client(new_fd);
		close(new_fd);
#else
		handle_gossip_client(server_socket);
#endif
	}

	close(server_socket);
	return NULL;
}
