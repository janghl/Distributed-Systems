#pragma once

#include <vector>
#include <string>
#include <iostream>

#include "name_node_block_report_manager.hpp"

extern BlockManager* block_manager_global;

void launch_name_node_server(int port, pthread_t& thread, BlockManager* block_manager);
void handle_name_node_client(int client_fd);
void* run_name_node_server(void* socket_ptr);

