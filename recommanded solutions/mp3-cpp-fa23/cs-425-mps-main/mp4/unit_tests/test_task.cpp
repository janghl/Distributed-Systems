#include <arpa/inet.h>
#include <sys/stat.h>

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <string>
#include <iostream>

#include "task.hpp"
#include "utils.hpp"
#include "../mp3/utils.hpp"
#include "../mp3/unit_tests/test_includes.hpp"

static const std::string executable = "exe.exe";
static const std::string input_file_prefix = "task_input.txt";
static const std::vector<std::string> input_keys = {"A", "B-key", "C"};
static const std::vector<std::string> input_files = {"A-file", "B-text-file", "C"};
static const std::vector<uint32_t> input_start_range = {342, 552, 12};
static const std::vector<uint32_t> input_end_range = {1093424, 43241, 14};
static const uint32_t task_id = 1032;
uint32_t self_machine_number;

Task init_maple_task(uint32_t& serialzied_size) {
	serialzied_size = (5 * sizeof(uint32_t)) + executable.length() + (3 * sizeof(uint32_t) * input_files.size());
	for (const std::string& file : input_files) {
		serialzied_size += file.length();
	}
	return Task(task_id, JobType::MAPLE,  executable, input_files, input_start_range, input_end_range);
}

Task init_juice_task(uint32_t& serialzied_size) {
	serialzied_size = (5 * sizeof(uint32_t)) + executable.length() + input_file_prefix.length() + (sizeof(uint32_t) * input_keys.size());
	for (const std::string& key : input_keys) {
		serialzied_size += key.length();
	}
	return Task(task_id, JobType::JUICE, executable, input_keys, input_file_prefix);
}

void test_maple_task_serialization() {
	uint32_t expected_serialized_size;
	Task task = init_maple_task(expected_serialized_size);
	assert(task.struct_serialized_size() == expected_serialized_size);

	int server_socket = setup_tcp_server_socket(2023);
	int client_socket = get_tcp_socket_with_node(self_machine_number, 2023);
	int client_fd = accept(server_socket, NULL, NULL);

	assert(task.serialize(client_socket) == expected_serialized_size + sizeof(uint32_t));

	Task deserialized = Task::deserialize(client_fd, "TEST_MAPLE_TASK");
	assert(task == deserialized);

	close(client_fd);
	close(client_socket);
	close(server_socket);
}


void test_juice_task_serialization() {
	uint32_t expected_serialized_size;
	Task task = init_juice_task(expected_serialized_size);
	assert(task.struct_serialized_size() == expected_serialized_size);

	int server_socket = setup_tcp_server_socket(2023);
	int client_socket = get_tcp_socket_with_node(self_machine_number, 2023);
	int client_fd = accept(server_socket, NULL, NULL);
	assert(task.serialize(client_socket) == expected_serialized_size + sizeof(uint32_t));

	Task deserialized = Task::deserialize(client_fd, "TEST_JUICE_TASK");
	assert(task == deserialized);

	close(client_fd);
	close(client_socket);
	close(server_socket);
}

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Invalid program arguments. Run as " << argv[0] << " [id of current machine]" << std::endl;
		return 0;
	}

	self_machine_number = std::stoi(argv[1]);
	test_juice_task_serialization();
	test_maple_task_serialization();
	return 0;
}
