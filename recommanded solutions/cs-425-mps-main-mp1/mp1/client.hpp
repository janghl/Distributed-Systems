#pragma once

ssize_t send_grep_command_to_socket(const std::string& command, int server_socket);
void send_grep_command_to_cluster(const std::string& command, std::ostream& outstream);
