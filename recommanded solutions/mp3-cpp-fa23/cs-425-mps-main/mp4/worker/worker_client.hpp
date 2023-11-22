#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "../task.hpp"
#include "../job_type.hpp"
#include "../../mp3/response.hpp"
#include "../utils.hpp"
#include "../../mp3/utils.hpp"

//extern uint32_t name_node_id;
void launch_client(uint32_t name_node_server_port_cpy);
void handle_client_command(Task task, std::string command);
