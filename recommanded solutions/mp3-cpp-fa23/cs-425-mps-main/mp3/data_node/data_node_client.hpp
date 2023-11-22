#pragma once

#include <vector>
#include <string>
#include <iostream>

#include "operation_type.hpp"
#include "../mp2/membership_list_manager.hpp"

class Request; // forward declaration
void launch_client(MembershipListManager* membership_list_ptr, uint32_t name_node_server_port_cpy);
Request* create_request(OperationType operation, std::string command);
void* manage_client_request(void* request_ptr, uint32_t manage_client_request);
void handle_client_command(OperationType operation, std::string command);
void handle_client_command(OperationType operation, std::string command, uint32_t name_node_server_port);
