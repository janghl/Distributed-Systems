#pragma once

#include <vector>
#include <string>
#include <iostream>

void launch_gossip_server(int port, pthread_t& thread);
void handle_gossip_client(int client_fd);
void* run_gossip_server(void* socket_ptr);
