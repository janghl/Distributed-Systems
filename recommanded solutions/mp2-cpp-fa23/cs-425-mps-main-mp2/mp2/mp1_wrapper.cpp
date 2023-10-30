#include <arpa/inet.h>

#include <cstring>
#include <iostream>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <chrono>

#include "../mp1/utils.hpp"
#include "utils.hpp"
#include "../mp1/client.hpp"
#include "../mp1/server.hpp"
#include "membership_list_manager.hpp"
#include "mp1_wrapper.hpp"

int server_port = grep_server_port;

void* run_server_wrapper(void* server_socket) {
	log_thread_started("grep server");
	run_server(server_socket);
	return NULL;
}

void launch_grep_server(pthread_t* grep_server_thread) {
	server_port = grep_server_port;
	int server_socket = setup_tcp_server_socket(server_port);
	pthread_create(grep_server_thread, NULL, run_server_wrapper, reinterpret_cast<void*>(server_socket));
}
