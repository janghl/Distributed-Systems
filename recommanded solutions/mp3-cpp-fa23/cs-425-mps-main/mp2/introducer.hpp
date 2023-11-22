#pragma once

#include <vector>
#include <string>
#include <iostream>

void launch_introducer_server(int port, pthread_t& thread);
void handle_new_node(int client_fd);
void send_self_information_to_introducer(int socket, uint32_t machine_id, uint32_t machine_version);
void get_introducer_information(int socket);
bool init_self_using_introducer(uint32_t machine_id, uint32_t machine_version, int introducer_machine_id, int introducer_port);
void* run_introducer_server(void* socket_ptr);
