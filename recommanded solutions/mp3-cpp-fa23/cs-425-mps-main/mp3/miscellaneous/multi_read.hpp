#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <unistd.h>
#include <string>
#include <iostream>

#include "../mp1/utils.hpp"
#include "../mp2/utils.hpp"
#include "utils.hpp"
#include "multi_read.hpp"
#include "data_node_client.hpp"

void launch_multi_read_server(int port, pthread_t& thread);
void handle_read_request(int client_fd);
void* run_multi_read_server(void* socket_ptr);
void send_multi_read(std::string local_file, std::string sdfs_file, const std::vector<int>& node_ids, int multi_read_port);