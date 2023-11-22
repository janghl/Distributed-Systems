#pragma once

#include <vector>
#include <string>
#include <iostream>

void launch_name_node_identifier_server(int port, pthread_t& thread);
void send_name_node_id(int client_fd);
void get_name_node_id(int introducer_machine_id, int name_node_identifier_port);
void* run_name_node_identifier_server(void* socket_ptr);
