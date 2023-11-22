#include <unistd.h>
#include <fstream>
#include <iostream>
#include <atomic>
#include <sys/stat.h>

#include "data_node_client.hpp"
#include "data_node_server.hpp"
#include "request.hpp"
#include "response.hpp"
#include "../../mp2/utils.hpp"
#include "../../mp2/membership_list_manager.hpp"
#include "test_includes.hpp"

uint32_t self_machine_number;
std::atomic<uint32_t> name_node_id;

std::string file_system_root_dir = "test_block_report_server_fs_dir";
std::string local_file_data = "hello world\nI am a file that data node test will upload to our SDFS";
std::string local_file_name = "test_write_local_file.txt";
std::string local_file_copy_name = "test_read_into_local_file.txt";
std::string sdfs_file_name = "dummy_write_file.txt";
int name_node_server_socket;

#define NAME_NODE_PORT 1101
#define DATA_NODE_PORT 2202

int data_node_server_port = DATA_NODE_PORT;
int name_node_server_port = NAME_NODE_PORT;

void test_write_request() {
	// act as data node client and send write request to name node
	handle_client_command(OperationType::WRITE, "put " + local_file_name + " " + sdfs_file_name);

	// act as name node and check data node sent a request
	Request request;
	int new_fd = accept(name_node_server_socket, NULL, NULL);
	get_request_over_socket(new_fd, request, "TEST");
	char* data = new char[local_file_data.length()];
	memcpy(data, local_file_data.c_str(), local_file_data.length());
	Request expected_request = Request(OperationType::WRITE, sdfs_file_name, local_file_data.length(), 0, nullptr);
	assert(expected_request == request);

	{
		std::ifstream f(file_system_root_dir + "/" + sdfs_file_name);
		assert(!f.good());
	}

	// act as name node and forward to request to data node server (aka replica)
	request.set_update_counter(1);
	int data_node_server_socket = get_tcp_socket_with_node(self_machine_number, DATA_NODE_PORT);
	send_request_over_socket(data_node_server_socket, request, "TEST: ", send_request_data_from_socket);

	// act as name node and get the response from data node server (aka replica) on the write request
	uint32_t i;
	Response* response = get_response_over_socket(data_node_server_socket, i);
	WriteSuccessReadResponse expected_response = WriteSuccessReadResponse(self_machine_number, OperationType::WRITE, 
			local_file_data.length(), sdfs_file_name, 1);
	assert(expected_response == *reinterpret_cast<WriteSuccessReadResponse*>(response));

	{
		std::ifstream f(file_system_root_dir + "/" + sdfs_file_name);
		assert(f.good());
	}

	// act as name node and send acknowledgment of write to the data node client
	if (dynamic_cast<WriteSuccessReadResponse*>(response)) {
		send_response_over_socket(new_fd, *reinterpret_cast<WriteSuccessReadResponse*>(response), "TEST: ");
	} else {
		send_response_over_socket(new_fd, *response, "TEST: ");
	}
	close(new_fd);
	close(data_node_server_socket);
}

// void test_read_request() {
// 	// act as data node client and send read request to name node
// 	handle_client_command(OperationType::READ, "get " + local_file_copy_name + " " + sdfs_file_name);

// 	// act as name node and check data node sent a request
// 	Request request;
// 	int new_fd = accept(name_node_server_socket, NULL, NULL);
// 	get_request_over_socket(new_fd, request);
// 	Request expected_request = Request(OperationType::READ, sdfs_file_name, nullptr);
// 	assert(expected_request == request);

// 	{
// 			std::ifstream f(local_file_copy_name);
// 			assert(!f.good());
// 	}

// 	// act as name node and forward to request to data node server (aka replica)
// 	int data_node_server_socket = get_tcp_socket_with_node(self_machine_number, DATA_NODE_PORT);
// 	send_request_over_socket(data_node_server_socket, request);

// 	// act as name node and get the response from data node server (aka replica) on the read request
// 	Response* response = get_response_over_socket(data_node_server_socket);
// 	char* data = new char[local_file_data.length()];
// 	memcpy(data, local_file_data.c_str(), local_file_data.length());
// 	WriteSuccessReadResponse expected_response = WriteSuccessReadResponse(self_machine_number, OperationType::READ,
// 					local_file_data.length(), sdfs_file_name, 1, local_file_data.length(), data);
// 	assert(expected_response == *reinterpret_cast<WriteSuccessReadResponse*>(response));

// 	{
// 			std::ifstream f(local_file_copy_name);
// 			assert(!f.good());
// 	}

// 	// act as name node and send acknowledgment of read to the data node client
// 	if (dynamic_cast<WriteSuccessReadResponse*>(response)) {
// 		send_response_over_socket(new_fd, *reinterpret_cast<WriteSuccessReadResponse*>(response));
// 	} else {
// 		send_response_over_socket(new_fd, *response);
// 	}
// 	close(new_fd);
// 	close(data_node_server_socket);
// 	sleep(2);
// 	{
// 			std::ifstream f(local_file_copy_name);
// 			assert(f.good());
// 	}
// }

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Invalid program arguments. Run as " << argv[0] << " [id of current machine]" << std::endl;
		return 0;
	}
	mkdir(file_system_root_dir.c_str(), 0700);
	std::remove(local_file_copy_name.c_str());

	std::ofstream file(local_file_name, std::ofstream::out | std::ofstream::trunc);
	if (!file) {
		std::cout << "Could not open file for writing" << std::endl;
		std::perror("ofsteam open failed");
		return -1;
	}
	file.write(local_file_data.c_str(), local_file_data.length());
	file.close();

	self_machine_number = std::stoi(argv[1]);
	name_node_id.store(self_machine_number);

	name_node_server_socket = setup_tcp_server_socket(NAME_NODE_PORT);

	BlockReport* block_report = new BlockReport(file_system_root_dir);
	membership_list_manager.init_self(self_machine_number, 0, block_report);

	launch_client(&membership_list_manager, NAME_NODE_PORT);

	pthread_t data_node_server_thread;
	launch_data_node_server(DATA_NODE_PORT, data_node_server_thread, block_report);

	test_write_request();
	// test_read_request();
	//test_delete_request();


	close(name_node_server_socket);

	return 0;
}
