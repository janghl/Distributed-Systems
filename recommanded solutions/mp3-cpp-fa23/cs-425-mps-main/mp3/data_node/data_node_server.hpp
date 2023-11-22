#pragma once

#include <vector>
#include <string>
#include <iostream>

#include "data_node_block_report_manager.hpp"
#include "request.hpp"

extern BlockReport* block_report_global;
extern uint32_t self_machine_id;

void launch_data_node_server(int port, pthread_t& thread, BlockReport* block_report_ptr);
void handle_data_node_write(int client_fd, Request& client_request, BlockReport* block_report);
void handle_data_node_read(int client_fd, Request& client_request, BlockReport* block_report);
void handle_data_node_delete(int client_fd, Request& client_request, BlockReport* block_report);
void handle_data_node_client(int client_fd);
void* run_data_node_server(void* socket_ptr);
