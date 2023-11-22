#include <fstream>
#include <unistd.h>
#include <vector>
#include <queue>
#include <iostream>
#include <cassert>
#include <sys/stat.h>

#include "worker_server.hpp"
#include "utils.hpp"
#include "task.hpp"
#include "../mp3/response.hpp"
#include "../mp3/request.hpp"
#include "../mp3/utils.hpp"
#include "../mp3/operation_type.hpp"
#include "../mp3/unit_tests/test_includes.hpp"

uint32_t self_machine_number;
std::atomic<uint32_t> name_node_id;

int data_node_server_port = 2022;
int maple_juice_leader_server_port = 4044;
int worker_server_port = 3033;

static const uint32_t maple_task_id = 101;
static const std::string maple_exe_file = "maple_test_worker_server.o";
static const std::vector<std::string> maple_input_files = {"maple_test_worker_server_file_1.txt", "maple_test_worker_server_file_2.txt"};
static const std::vector<uint32_t> maple_start_lines = {0, 5};
static const std::vector<uint32_t> maple_end_lines = {3, 190};
std::vector<std::string> maple_expected_output_files;

static const uint32_t juice_task_id = 232;
static const std::string juice_exe_file = "juice_test_worker_server.o";
std::string intermediate_file = "_maple_output_maple_test_worker_server_file_";
static const std::vector<std::string> juice_keys = {"1.txt_0_3", "2.txt_5_190"};
std::string juice_expected_output_file;

std::vector<std::string> expected_maple_juice_leader_file;
std::vector<OperationType> expected_maple_juice_leader_operation;
uint32_t first_write_position;
uint32_t num_name_node_requests;

void handle_maple_juice_leader_client(int new_fd) {
	Request client_request;
        if (!get_request_over_socket(new_fd, client_request, "MOCK_NAME_NODE_SERVER: ")) {
		assert(false && "worker server sent malformed request");
        }
	auto it = std::find(expected_maple_juice_leader_file.begin(), expected_maple_juice_leader_file.end(), client_request.get_file_name());
	if (it == expected_maple_juice_leader_file.end()) {
		assert(false && "worker server performed operation on unexpected file");
	}
	std::string expected_file = *it;
	uint32_t index = it - expected_maple_juice_leader_file.begin();
	OperationType expected_operation = expected_maple_juice_leader_operation[index];

	num_name_node_requests += 1;

	assert(client_request.get_request_type() == expected_operation);
	assert(client_request.get_file_name() == expected_file);

	if (expected_operation == OperationType::READ) {
		assert(index < first_write_position);
		uint32_t file_data_size;
		char* file_data;

		std::fstream file;
		std::string dir = (maple_exe_file == expected_file || juice_exe_file == expected_file) ? "./unit_tests/" : "./worker_temp_files/";
		file.open(dir + expected_file, std::ofstream::in);
		if (!file) {
			file_data_size = expected_file.length();
			file_data = new char[file_data_size];
			memcpy(file_data, expected_file.c_str(), file_data_size);
		} else {
			file.seekp(0, std::ios::end);
			file_data_size = file.tellp();
			file.seekg(0, std::ios::beg);

			file_data = new char[file_data_size];
			file.read(file_data, file_data_size);

			file.close();
		}

		std::cout << "read on file " << dir << expected_file << " : " << file_data << std::endl;

		WriteSuccessReadResponse response(self_machine_number, OperationType::READ, file_data_size,
                        expected_file, 1, file_data_size, file_data);
		send_response_over_socket(new_fd, response, "MOCK_NAME_NODE_SERVER: ");
		return;
	} else if (expected_operation == OperationType::WRITE) {
		assert(index >= first_write_position);
		WriteSuccessReadResponse response(self_machine_number, OperationType::WRITE, 1, expected_file, 1);
		send_response_over_socket(new_fd, response, "MOCK_NAME_NODE_SERVER: ");
                return;
	}

	assert(false && "Mock name node only supports WRITE and READ operations");
}

void* mock_maple_juice_leader(void*) {
	int server_socket = setup_tcp_server_socket(maple_juice_leader_server_port);
	while (not end_session) {
		int new_fd = accept(server_socket, NULL, NULL);
		if (new_fd < 0) {
			perror("NameNode: Could not accept client");
			continue;
		}
		handle_maple_juice_leader_client(new_fd);
	}
	close(server_socket);
	return NULL;
}

void test_maple_task() {
	num_name_node_requests = 0;
	expected_maple_juice_leader_file.push_back(maple_exe_file);
	expected_maple_juice_leader_operation.push_back(OperationType::READ);

	for (const std::string& file : maple_input_files) {
		expected_maple_juice_leader_file.push_back(file);
		expected_maple_juice_leader_operation.push_back(OperationType::READ);
	}

	first_write_position = expected_maple_juice_leader_file.size();
	for (const std::string& file : maple_expected_output_files) {
                expected_maple_juice_leader_file.push_back(file);
                expected_maple_juice_leader_operation.push_back(OperationType::WRITE);
        }

	Task maple_task(maple_task_id, JobType::MAPLE, maple_exe_file, maple_input_files, maple_start_lines, maple_end_lines);
	int worker_socket = get_tcp_socket_with_node(self_machine_number, worker_server_port);
	send_task_over_socket(worker_socket, maple_task, "TEST_WORKER_SERVER:");

	// TODO: Wait to hear ACK from worker server
	


	while (num_name_node_requests < expected_maple_juice_leader_file.size()) {
		sleep(1);
	}

	expected_maple_juice_leader_file.clear();
	expected_maple_juice_leader_operation.clear();
	num_name_node_requests = 0;
}

void test_juice_task() {
        num_name_node_requests = 0;
        expected_maple_juice_leader_file.push_back(juice_exe_file);
        expected_maple_juice_leader_operation.push_back(OperationType::READ);

        for (const std::string& key : juice_keys) {
                expected_maple_juice_leader_file.push_back(intermediate_file + key);
                expected_maple_juice_leader_operation.push_back(OperationType::READ);
        }

        first_write_position = expected_maple_juice_leader_file.size();
	expected_maple_juice_leader_file.push_back(juice_expected_output_file);
	expected_maple_juice_leader_operation.push_back(OperationType::WRITE);

        Task maple_task(juice_task_id, JobType::JUICE, juice_exe_file, juice_keys, intermediate_file);
        int worker_socket = get_tcp_socket_with_node(self_machine_number, worker_server_port);
        send_task_over_socket(worker_socket, maple_task, "TEST_WORKER_SERVER:");

        // TODO: Wait to hear ACK from worker server


        while (num_name_node_requests < expected_maple_juice_leader_file.size()) {
                sleep(1);
        }

	expected_maple_juice_leader_file.clear();
	expected_maple_juice_leader_operation.clear();
	num_name_node_requests = 0;
}

void delete_dir_content(const std::filesystem::path& dir_path) {
	for (auto& path: std::filesystem::directory_iterator(dir_path)) {
		std::filesystem::remove_all(path);
	}
}

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Invalid program arguments. Run as " << argv[0] << " [id of current machine]" << std::endl;
		return 0;
	}
	for(uint32_t i = 0; i < maple_input_files.size(); ++i) {
		maple_expected_output_files.push_back(std::to_string(maple_task_id) + "_maple_output_" + maple_input_files[i] + "_" + 
				std::to_string(maple_start_lines[i]) + "_" + std::to_string(maple_end_lines[i]));
	}
	juice_expected_output_file = std::to_string(juice_task_id) + "_juice_output";
	intermediate_file = std::to_string(maple_task_id) + intermediate_file;
	for (const std::string& file : maple_expected_output_files) {
		std::remove(file.c_str());
	}
	for (const std::string& file : maple_input_files) {
		std::remove(file.c_str());
	}
	delete_dir_content("worker_temp_files");
	self_machine_number = std::stoi(argv[1]);
	name_node_id.store(self_machine_number);

	pthread_t worker_thread;
	launch_worker_server(worker_server_port, worker_thread, maple_juice_leader_server_port);

	pthread_t maple_juice_leader_thread;
	pthread_create(&maple_juice_leader_thread, NULL, mock_maple_juice_leader, NULL);

	test_maple_task();
	test_juice_task();

	return 0;
}
