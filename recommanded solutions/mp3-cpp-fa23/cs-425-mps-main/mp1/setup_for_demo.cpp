#include <iostream>
#include <string>
#include <vector>

#include "utils.hpp"

std::vector<std::string> machine_ips;

int server_port = 4091;
std::string user = "gaurip2";
std::string machine_id = "-1";

// Returns name of machine with number machine_number
std::string get_machine_name(int machine_number) {
	std::string machine_number_str = std::to_string(machine_number);
	// machine_number_str needs to have length 2
	if (machine_number_str.length() < 2) {
		machine_number_str = "0" + machine_number_str;
	} else if (machine_number_str.length() > 2) {
		std::cerr << "Invalid machine number; machine number has more than 2 digits" << std::endl;
		exit(-1);
	}
	return "fa23-cs425-72" + machine_number_str + ".cs.illinois.edu";
}

void setup_distributed_test(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);

		std::string machine_ssh = user + "@" + machine_name;

		// clone/pull repo
		std::string gitclone = "git clone https://" + user + ":glpat-5Xo3TVm6xdHLTXrB2DVz@gitlab.engr.illinois.edu/gaurip2/cs-425-mp-1.git";
		std::string update_local_command = "ssh " + machine_ssh + " \"(cd cs-425-mp-1 && git pull) || " + gitclone + "\"";
		system(update_local_command.c_str());
	}
}

// Copies vm[i].log files to test folder on ith machine, for all i
void setup_provided_logs(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = user + "@" + machine_name;

		std::string copy_files = "provided_logs/vm" + std::to_string(machine_number) + ".log";
		std::string copy_logs_command = "scp " + copy_files + " " + machine_ssh + ":cs-425-mp-1/test/";
		system(copy_logs_command.c_str());
	}
}

// Delete vm[i].log and all_vm_logs.log files in test folder from ith machine, for all i
void delete_provided_logs(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);

		std::string machine_ssh = user + "@" + machine_name;

		std::string delete_vm_logs = "ssh " + machine_ssh + " rm cs-425-mp-1/test/vm*.log cs-425-mp-1/test/all_vm_logs.log";
		system(delete_vm_logs.c_str());
	}
}

// Delete all files in test folder from ith machine, for all i
void teardown_distributed_test(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = user + "@" + machine_name;
		std::string delete_test_dir_command = "ssh " + machine_ssh + " rm cs-425-mp-1/test/*";
		system(delete_test_dir_command.c_str());
	}
}

// Kill any process with name server_client on all machines
void terminate_process_on_distributed(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = user + "@" + machine_name;
		std::string terminate_process_command = "ssh " + machine_ssh + " pkill server_client &> /dev/null";
		system(terminate_process_command.c_str());
	}
}

int main() {
	std::vector<int> machines;
	machines.push_back(1);
	machines.push_back(2);
	machines.push_back(3);
	machines.push_back(4);
	machines.push_back(5);
	machines.push_back(6);
	machines.push_back(7);
	machines.push_back(8);
	machines.push_back(9);
	machines.push_back(10);

	terminate_process_on_distributed(machines);
	teardown_distributed_test(machines);
	setup_distributed_test(machines);
	setup_provided_logs(machines);
}
