#include <arpa/inet.h>
#include <sys/stat.h>

#include <chrono>
#include <cstdio>
#include <pthread.h>
#include <unordered_set>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <fstream>
#include <regex>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <cstdlib>

// variables for testing
std::string github_user;
std::string github_token;
std::string mp_directory = "cs-425";
std::string mp2_directory = mp_directory + "/mp2";
std::string mp2_logs_directory = mp2_directory + "/test";

// GENERAL HELPERS
// Returns "fa23-cs425-72[machine_id formatted as a string of length 2].cs.illinois.edu"
std::string get_machine_name(uint32_t machine_id) {
        std::string machine_id_str = std::to_string(machine_id);
        if (machine_id_str.length() < 2) {
                machine_id_str = "0" + machine_id_str;
        } else if (machine_id_str.length() > 2) {
                std::cerr << "Invalid machine number; machine number has more than 2 digits" << std::endl;
                exit(-1);
        }
        return "fa23-cs425-72" + machine_id_str + ".cs.illinois.edu";
}

// Concatinates all elements of machines vector into string (space-seperate) and returns the string
std::string get_space_seperated_machine_names(const std::vector<int>& machines) {
        std::string space_seperate_machine_names = "";
        for (const int machine_number : machines) {
                std::string machine_name = get_machine_name(machine_number);
                space_seperate_machine_names += machine_name + " ";
	}

	return space_seperate_machine_names;
}

// Sshes into every machine in machines and does a git pull (clones the repo as well if it isn't present on the machine)
void pull_from_repo_on_all_machines(const std::vector<int>& machines) {
	std::string gitclone = "git clone https://" + github_user + ":" + github_token + "@gitlab.engr.illinois.edu/" + 
					github_user + "/cs-425-mps.git " + mp_directory; //  clones into cs-425-mps folder
        for (const int machine_number : machines) {
                std::string machine_name = get_machine_name(machine_number);
                std::string machine_ssh = github_user + "@" + machine_name;

                // pull/clone repo
                std::string update_local_command = "ssh " + machine_ssh + " \"(cd " + mp_directory + " && git pull) || " + gitclone + "\"";
                system(update_local_command.c_str());
        }
}

// Deletes all files in cs-425-mps/mp2/test (i.e. mp2_logs_directory) directory
void delete_logs(const std::vector<int>& machines) {
        for (const int machine_number : machines) {
                std::string machine_name = get_machine_name(machine_number);
                std::string machine_ssh = github_user + "@" + machine_name;
                std::string delete_logs_command = "ssh " + machine_ssh + " rm " + mp2_logs_directory + "/*";
                system(delete_logs_command.c_str());
        }
}

// Creates directory cs-425-mps/mp2/test (i.e. mp2_logs_directory) 
void setup_logs(const std::vector<int>& machines) {
        for (const int machine_number : machines) {
                std::string machine_name = get_machine_name(machine_number);
                std::string machine_ssh = github_user + "@" + machine_name;

                // create test directory on machine
                std::string create_test_dir_command = "ssh " + machine_ssh + " mkdir " + mp2_logs_directory;
                system(create_test_dir_command.c_str());
        }
}

std::string process_name = "membership_manager";
std::string compile_program = "g++ -std=c++17 -g2 gossip_client.cpp gossip_server.cpp introducer.cpp main.cpp mp1_wrapper.cpp utils.cpp ../mp1/client.cpp ../mp1/server.cpp ../mp1/utils.cpp -I ../mp1 -lpthread -o " + process_name; // + " -DFALSE_POSTIVE_RATE -DFAILED_PRINT"; 

// Compiles mp2 on machine identified by machine_name
void* compile_process(void* machine_id_ptr) {
	int machine_id = static_cast<int>(reinterpret_cast<intptr_t>(machine_id_ptr));
	std::string machine_name = get_machine_name(machine_id);
        std::string machine_ssh = github_user + "@" + machine_name;
        std::string launch_process_command = "ssh " + machine_ssh + " \"cd " + mp2_directory + " && " + compile_program + "\"";
	std::cout << "Running: " << launch_process_command << std::endl;
        system(launch_process_command.c_str());

	return NULL;
}

void compile_process_on_all_machines(const std::vector<int>& machines) {
	std::vector<pthread_t> threads;
        for (const int machine_number : machines) {
		threads.emplace_back();
		pthread_create(&threads.back(), NULL, compile_process, reinterpret_cast<void*>(machine_number));
	}
	for (const pthread_t& thread : threads) {
		pthread_join(thread, NULL);
	}
}

// For every machine in machines, kills all processes with name membership_manager (i.e. process_name). Joins all threads in processes as
// well
void terminate_process_on_all_machines(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = github_user + "@" + machine_name;
		std::string kill_command = "\"ps -ef | grep " + process_name + " | grep -v test | awk '{print \\$2}' | xargs kill -9\"";
		std::string terminate_process_command = "ssh " + machine_ssh + " " + kill_command;
		std::cout << terminate_process_command << std::endl;
		system(terminate_process_command.c_str());
	}
}

int main() {
	github_user = getenv("GITHUB_USERNAME");
	github_token = getenv("GITHUB_TOKEN");

	std::vector<int> machines;
	for(int i = 1; i <= 10; ++i) {
		machines.push_back(i);
	}

	terminate_process_on_all_machines(machines);
	delete_logs(machines);

	std::string space_seperate_machine_names = get_space_seperated_machine_names(machines);
	pull_from_repo_on_all_machines(machines);
	setup_logs(machines);

	compile_process_on_all_machines(machines);

	std::cout << "All machine names space seperated: " << std::endl << space_seperate_machine_names << std::endl;

	return 0;
}
