#include <filesystem>
#include <vector>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>

#include "utils.hpp"
#include "worker_server.hpp"
#include "task.hpp"
#include "job_type.hpp"

#include "../mp3/data_node/data_node_client.hpp"
#include "../mp3/request.hpp"
#include "../mp2/utils.hpp"

using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

static const std::string temp_folder = "worker_temp_files";
uint32_t maple_juice_leader_server_port_ref;

#define TEMP_FILE_PATH(file) (temp_folder + "/" + file)
#define GET_FILE_FAILED reinterpret_cast<std::thread*>(0x4B1D)

void launch_worker_server(int port, pthread_t& thread, uint32_t maple_juice_leader_server_port) {
	mkdir(temp_folder.c_str(), 0700);
	int server_socket = setup_tcp_server_socket(port);
	maple_juice_leader_server_port_ref = maple_juice_leader_server_port;
	pthread_create(&thread, NULL, run_worker_server, reinterpret_cast<void*>(server_socket));
}

std::thread* get_file_from_sdfs(std::string file) {
	std::string command = "get " + file + " " + TEMP_FILE_PATH(file);
	Request* request = create_request(OperationType::READ, command);
#ifdef DEBUG_MP4_WORKER_SERVER
	std::cout << "WORKER_SERVER: " << "created request class for command `" << command << "`" << std::endl;
#endif
	if (request->get_request_type() == OperationType::INVALID) {
		std::cout << "WORKER_SERVER: " << "Cannot execute request; type invalid" << std::endl;
		return GET_FILE_FAILED; 
	}

#ifdef DEBUG_MP4_WORKER_SERVER
	std::cout << "WORKER_SERVER: " << "launching thread to manage the request" << std::endl;
#endif
	return new std::thread(&manage_client_request, request, maple_juice_leader_server_port_ref);
}

std::thread* get_executable(std::string executable_name) {
	if (access(TEMP_FILE_PATH(executable_name).c_str(), F_OK) == 0) {
		return nullptr;
	}
	return get_file_from_sdfs(executable_name);
}

bool get_files(std::string executable_name, std::string input_file_prefix, std::vector<std::string> inputs) {
	std::vector<std::thread*> threads;
	threads.push_back(get_executable(executable_name));
	for (const std::string& input : inputs) {
		threads.push_back(get_file_from_sdfs(input_file_prefix + input));
	}

	bool got_all_files = true;
	for (std::thread* thread_ptr : threads) {
		if (thread_ptr == nullptr) {
			continue;
		}
		if (thread_ptr == GET_FILE_FAILED) {
			got_all_files = false;
			continue;
		}
		thread_ptr->join();
	}

	system(("chmod 755 " + TEMP_FILE_PATH(executable_name)).c_str());

	return got_all_files;
}

void handle_worker_maple(int client_fd, Task& task) {
	std::string file_lines = "";
	std::vector<std::string> inputs = task.get_inputs();
	std::vector<uint32_t> starting_line = task.get_input_range_start();
	std::vector<uint32_t> ending_line = task.get_input_range_end();
	for (uint32_t i = 0; i < inputs.size(); ++i) {
		file_lines += " ";
		file_lines += TEMP_FILE_PATH(inputs[i]) + ":" + std::to_string(starting_line[i]) + ":" + std::to_string(ending_line[i]);
	}
	std::string system_command = TEMP_FILE_PATH(task.get_executable_file_name()) + " " + std::to_string(task.get_task_id()) + file_lines;
	int system_return = system(system_command.c_str());

#ifdef DEBUG_MP4_WORKER_SERVER
	std::cout << "Executed command " << system_command << std::endl;
	std::cout << "Maple execution returned: " << system_return << std::endl;
#endif

	std::string maple_output_file_prefix =  std::to_string(task.get_task_id()) + "_maple_output";
	for (const auto& dir_entry : recursive_directory_iterator(temp_folder)) {
		std::string path = dir_entry.path().relative_path();  
		int index = path.find(maple_output_file_prefix);
		if (index < 0) {
#ifdef DEBUG_MP4_WORKER_SERVER
                	std::cout << "File " << path << " is not a maple output" << std::endl;
#endif
			continue;
		}
#ifdef DEBUG_MP4_WORKER_SERVER
                std::cout << "File " << path << " is a maple output; sending it to sdfs" << std::endl;
#endif
		handle_client_command(OperationType::WRITE, "put " + path + " " + path.substr(index), maple_juice_leader_server_port_ref);
	}
}

void handle_worker_juice(int client_fd, Task& task) {
	std::string key_str = "";                                                                                                                
	std::vector<std::string> keys = task.get_inputs();                                                                                        
	for (uint32_t i = 0; i < keys.size(); ++i) {                                                                                              
		key_str += " " + keys[i];
	}
	std::string system_command = TEMP_FILE_PATH(task.get_executable_file_name()) + " " + std::to_string(task.get_task_id()) + " " +
					TEMP_FILE_PATH(task.get_input_file_prefix()) + key_str;
	int system_return = system(system_command.c_str());

#ifdef DEBUG_MP4_WORKER_SERVER
        std::cout << "Executed command " << system_command << std::endl;
        std::cout << "Juice execution returned: " << system_return << std::endl;
#endif

	std::string juice_output_file_name = std::to_string(task.get_task_id()) + "_juice_output";
	handle_client_command(OperationType::WRITE, "put " + TEMP_FILE_PATH(juice_output_file_name) + " " + juice_output_file_name, 
			maple_juice_leader_server_port_ref);
}

void handle_worker_client(int client_fd) {
	Task client_task;
#ifdef DEBUG_MP4_WORKER_SERVER
	std::cout << "WORKER_SERVER: " << "Getting client task..." << std::endl;
#endif
	if (!get_task_over_socket(client_fd, client_task, "WORKER_SERVER: ")) {
		std::cout << "WORKER_SERVER: " << "Could not parse client task" << std::endl;
		// TODO: Send NAK to client_fd
		close(client_fd);
		return;
	}

	// get the executable and input files SDFS
	if (!get_files(client_task.get_executable_file_name(), client_task.get_input_file_prefix(), client_task.get_inputs())) {
		std::cout << "WORKER_SERVER: Could not get executable and/or the input files. Cannot process task" << std::endl;
		// TODO: Send NAK to client_fd
		close(client_fd);
		return;
	}

	// process the task
	if (client_task.get_task_type() == JobType::MAPLE) {
#ifdef DEBUG_MP4_WORKER_SERVER
		std::cout << "WORKER_SERVER: " << "Client sent maple task" << std::endl;
#endif
		handle_worker_maple(client_fd, client_task);
	} else if (client_task.get_task_type() == JobType::JUICE) {
#ifdef DEBUG_MP4_WORKER_SERVER
		std::cout << "WORKER_SERVER: " << "Client sent juice task" << std::endl;
#endif
		handle_worker_juice(client_fd, client_task);
	} else {
		std::cout << "WORKER_SERVER: " << "Invalid task type: " << static_cast<uint32_t>(client_task.get_task_type()) << std::endl;       
	}
	// TODO: Send ACK/NAK to client_fd
	close(client_fd);
}

void* run_worker_server(void* socket_ptr) {
	log_thread_started("worker server");
	int server_socket = static_cast<int>(reinterpret_cast<intptr_t>(socket_ptr));

	while(not end_session) {
		int new_fd = accept(server_socket, NULL, NULL);
#ifdef DEBUG_MP4_WORKER_SERVER
		std::cout << "WORKER_SERVER: " << "accpeted a new client connection on fd: " << new_fd << std::endl;
#endif
		if (new_fd < 0) {
			perror("Could not accept client");
			break;
		}

		std::thread client_handler = std::thread(&handle_worker_client, new_fd);
		client_handler.detach(); // detach so the thread can run independently of the main thread
	}

	close(server_socket);
	return NULL;
}


