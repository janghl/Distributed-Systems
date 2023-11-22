#pragma once

#include <string>

void handle_client(int client_socket);
std::string execute_grep_command(std::string & command);
void* run_server(void*);
