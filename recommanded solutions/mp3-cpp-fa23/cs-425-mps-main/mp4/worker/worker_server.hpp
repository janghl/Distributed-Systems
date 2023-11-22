#pragma once

#include <vector>
#include <string>
#include <iostream>

#include "task.hpp"

void launch_worker_server(int port, pthread_t& thread, uint32_t maple_juice_leader_server_port);
void handle_worker_maple(int client_fd, Task& task);
void handle_worker_juice(int client_fd, Task& task);
void handle_worker_client(int client_fd);
void* run_worker_server(void* socket_ptr);
