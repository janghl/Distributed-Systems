#include <iostream>
#include <unistd.h>
#include <fstream>
#include <ios>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include "worker_client.hpp"


std::vector<pthread_t> client_threads;
uint32_t leader_server_port_ref;


void launch_client(uint32_t leader_server_port_cpy) {
	leader_server_port_ref = leader_server_port_cpy;
}


Job* create_job(std::string command) {	
	
	//command: 
		//maple <maple_exe> <num_maples> <sdfs_intermediate_filename_prefix> <sdfs_src_directory>	 
		//juice <juice_exe> <num_juices> <sdfs_intermediate_filename_prefix> <sdfs_dest_filename> delete_input={0,1}
	std::istringstream iss(command);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    if (tokens.empty()) {
        return nullptr;
    }
	JobType job_type;
    std::string executable_file_name;
    uint32_t num_tasks;
    std::string input_file;
    std::string output_file;
	bool delete_flag;//for juice
	if (tokens[0] == "maple") {
        job_type = JobType::MAPLE;
    } else if (tokens[0] == "juice") {
        job_type = JobType::JUICE;
    } else {
		std::cout << "WORKER_CLIENT: " << "Cannot execute request; type invalid" << std::endl;
        return nullptr;
    }
	
    // Parsing based on job type
    try {
        if (job_type == JobType::MAPLE) {
            if (tokens.size() < 5) return nullptr;
            executable_file_name = tokens[1];
            num_tasks = std::stoul(tokens[2]);
            input_file = tokens[4] + "/" + tokens[3];// location + prefix, leader to find all files with this prefix         
            output_file = tokens[3];//prefix
        } else if (job_type == JobType::JUICE) {
            if (tokens.size() < 6) return nullptr;

            executable_file_name = tokens[1];
            num_tasks = std::stoul(tokens[2]);
            output_file = tokens[4];
            
            input_file = tokens[3] ; //prefix ，the file in sdfs
			if(token[5] == 1){
				delete_flag = true;
			}
			if(token[5] == 0){
				delete_flag = false;
			}
        }
    } catch (const std::exception& e) {
        std::cout << "Error parsing command: " << e.what() << std::endl;
        return nullptr;
    }

    //return new Job(0, job_type, num_tasks, executable_file_name, input_file, output_file, delete_flag);
	return new Job(0, job_type, num_tasks, executable_file_name, input_file, output_file);
}

    

int get_tcp_socket_with_leader() {
	uint32_t possible_leader_id = name_node_id.load();
	int socket = socket = get_tcp_socket_with_node(possible_leader_id, leader_server_port_ref);

	while (socket < 0) { // currently recorded leader has died
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // give time for a new leader to come up
		//possible_leader_id = membership_list->get_lowest_node_id();
		socket = get_tcp_socket_with_node(possible_leader_id, leader_server_port_ref);
	};
	name_node_id.store(possible_leader_id);
	return socket;
}

Response* try_sending_job(Job* job, int& socket) {
	socket = get_tcp_socket_with_leader();
#ifdef DEBUG_MP4_WORKER_CLIENT
	std::cout << "WORKER_CLIENT: " << "sending request to name node (node id: " << name_node_id << ")" << std::endl;
#endif
	std::string job_type_string = job->job_to_string(job->get_job_type());

	send_job_over_socket(socket, *job, job_type_string + " " + job->get_input_file());
#ifdef DEBUG_MP4_WORKER_CLIENT
	std::cout << "WORKER_CLIENT: " << "waiting to get name nodes response..." << std::endl;
#endif
	uint32_t payload_size;
	Response* response = get_response_over_socket(socket, payload_size);
#ifdef DEBUG_MP4_WORKER_CLIENT
	std::cout << "WORKER_CLIENT: " << "got response from name node" << std::endl;
#endif
	return response;
}

void* manage_client_request(Job* job) {
     

#ifdef DEBUG_MP4_WORKER_CLIENT
    std::cout << "WORKER_CLIENT: " << "starting to manage request" << std::endl;
#endif

    int socket;
    Response* response = try_sending_job(job, socket);

    while (response == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        response = try_sending_job(job, socket);
    }    

    if (response->get_status_code() < 0) {
        std::cout << job->job_to_string(job->get_job_type()) << " on file " << job->get_input_file()
                  << " failed with status code " << response->get_status_code() << std::endl;
    } 
	else{		
	#ifdef DEBUG_MP4_WORKER_CLIENT
			std::cout << "WORKER_CLIENT: " << "MAPLE request successfully; writing data to a local file" << std::endl;
	#endif
	}
	close(socket);
	delete job;
	delete response;
	return nullptr;
}

void handle_client_command(std::string command) {
	Job* job = create_job(command);
#ifdef DEBUG_MP4_WORKER_CLIENT
	std::cout << "WORKER_CLIENT: " << "created task class for command `" << command << "`" << std::endl;
#endif


#ifdef DEBUG_MP4_WORKER_CLIENT
	std::cout << "WORKER_CLIENT: " << "launching thread to manage the request" << std::endl;
#endif
	std::thread client_handler = std::thread(&manage_client_request, job);
	client_handler.detach(); // detach so the thread can run independently of the main thread
	// pthread_create(&client_threads.back(), NULL, manage_client_request, request);

	return;
}
