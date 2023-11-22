#include <arpa/inet.h>
#include <sys/stat.h>

#include <cassert>
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

#include "utils.hpp"
#include "client.hpp"
#include "server.hpp"

int server_port = 4091;
std::vector<std::string> machine_ips;
bool end_session = false;
std::string user = "gaurip2";
std::string machine_id = "-1";
bool should_generate_log_files = true;
bool should_remove_log_files = true;

int num_success_tests = 0;
int num_total_tests = 0;

class regex_wrapper {
	public:
		std::string regex_str;
		std::regex regex;

		regex_wrapper(std::string str) : regex_str(str), regex(std::regex(str)) {}
};

bool client_reply_matches(const std::vector<regex_wrapper>& expected_replies, int client_output_fd);

void test_client_server_connection() {
	mkdir("test", 0700);
	std::ofstream outfile ("test/1.log");
	std::string grep_result = "main\nmain test\n";
	outfile << grep_result << std::endl;
	outfile << "Main\nmAin test" << std::endl;
	outfile.close();

	pthread_t server_thread;
	pthread_create(&server_thread, NULL, run_server, NULL);

	usleep(1000000); // give server a seconds to setup

	machine_ips.push_back(std::string("127.0.0.1"));

	std::string test_message = "grep main";
	std::stringstream output_stream;
	send_grep_command_to_cluster(test_message, output_stream);

	std::string server_reply = output_stream.str();
	assert(server_reply == grep_result);

	end_session = true;

	machine_ips.clear();

	remove("test/1.log");
	remove("test");

	std::cout << "test_client_server_connection : " << "Success" << std::endl;
	++num_success_tests;
	++num_total_tests;
}

void test_machine_ip_generation() {
	char* machine_names[] = {"fa23-cs425-7201.cs.illinois.edu"};
	process_machine_names(1, machine_names);

	for (const std::string& ip : machine_ips) {
		std::cout << "got ip: " << ip << std::endl;
	}
	machine_ips.clear();
}

void generate_log_files(const std::string& machine_name, int num_files) {
	std::string generate_logs_command = "python3 ./log_generate.py " + machine_name + " " + std::to_string(num_files);
	system(generate_logs_command.c_str());
}

std::string get_machine_name(int machine_number) {
	std::string machine_number_str = std::to_string(machine_number);
	if (machine_number_str.length() < 2) {
		machine_number_str = "0" + machine_number_str;
	} else if (machine_number_str.length() > 2) {
		std::cerr << "Invalid machine number; machine number has more than 2 digits" << std::endl;
		exit(-1);
	}
	return "fa23-cs425-72" + machine_number_str + ".cs.illinois.edu";
}

std::string setup_distributed_test(const std::vector<int>& machines, int num_files) {
	std::string space_seperate_machine_names = "";
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		space_seperate_machine_names += machine_name + " ";

		std::string machine_ssh = user + "@" + machine_name;

		// clone/pull repo
		std::string gitclone = "git clone https://" + user + ":glpat-5Xo3TVm6xdHLTXrB2DVz@gitlab.engr.illinois.edu/gaurip2/cs-425-mp-1.git";
		std::string update_local_command = "ssh " + machine_ssh + " \"(cd cs-425-mp-1 && git pull) || " + gitclone + "\"";
		system(update_local_command.c_str());
	}
	return space_seperate_machine_names;
}

void delete_generated_logs(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = user + "@" + machine_name;
		std::string delete_test_dir_command = "ssh " + machine_ssh + " rm cs-425-mp-1/test/fa*.log";
		system(delete_test_dir_command.c_str());
	}
	machine_ips.clear();

}

void setup_generated_logs(const std::vector<int>& machines, int num_files) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = user + "@" + machine_name;

		// generate logs for each machine
		generate_log_files(machine_name, num_files);

		// create test directory on machine
		std::string create_test_dir_command = "ssh " + machine_ssh + " mkdir cs-425-mp-1/test";
		system(create_test_dir_command.c_str());

		// copy files from generated logs to test folder on machine
		std::string copy_files = "generated_logs/" + machine_name + ".*";
		std::string copy_logs_command = "scp -r " + copy_files + " " + machine_ssh + ":cs-425-mp-1/test/";
		system(copy_logs_command.c_str());
	}
}

void setup_provided_logs(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = user + "@" + machine_name;

		// also copy vm[i].log and all_vm_logs.log files
		std::string copy_files = "provided_logs/vm" + std::to_string(machine_number) + ".log";
		std::string copy_logs_command = "scp " + copy_files + " " + machine_ssh + ":cs-425-mp-1/test/";
		system(copy_logs_command.c_str());

		copy_files = "provided_logs/all_vm_logs.log";
		std::string create_test_result_dir_command = "ssh " + machine_ssh + " mkdir cs-425-mp-1/test_result";
		system(create_test_result_dir_command.c_str());
		copy_logs_command = "scp " + copy_files + " " + machine_ssh + ":cs-425-mp-1/test_result/";
		system(copy_logs_command.c_str());
	}
}

void delete_provided_logs(const std::vector<int>& machines) {
	std::string space_seperate_machine_names = "";
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		space_seperate_machine_names += machine_name + " ";

		std::string machine_ssh = user + "@" + machine_name;

		// also delete vm[i].log and all_vm_logs.log files
		std::string delete_vm_logs = "ssh " + machine_ssh + " rm cs-425-mp-1/test/vm*.log cs-425-mp-1/test/all_vm_logs.log";
		system(delete_vm_logs.c_str());
	}
}

void teardown_distributed_test(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = user + "@" + machine_name;
		std::string delete_test_dir_command = "ssh " + machine_ssh + " rm cs-425-mp-1/test/*";
		system(delete_test_dir_command.c_str());
	}
	machine_ips.clear();
}

std::string test_mode_compilation = "-D TESTING";
std::string compile_program = "g++ -std=c++14 main.cpp server.cpp client.cpp utils.cpp -lpthread -g -o server_client";
std::string testing_mode_compile_program = compile_program + " " + test_mode_compilation;
std::vector<pthread_t> processes;
int num_processes_running = 0;

void* exec_command(void* command) {
	char* s = (char*)(command);
	num_processes_running += 1;
	system(s);
	return NULL;
}

void compile_process(const std::string machine_name, bool test_mode = false) {
	std::string machine_ssh = user + "@" + machine_name;
	std::string launch_process_command = "ssh " + machine_ssh + " \"cd cs-425-mp-1 && " + 
		(test_mode ? testing_mode_compile_program : compile_program) + "\"";
	system(launch_process_command.c_str());
}

void start_process(const int machine_number, const std::string& space_seperate_machine_names, bool test_mode = false) {
	std::string machine_name = get_machine_name(machine_number);
	std::string machine_ssh = user + "@" + machine_name;
	std::string ignore_stdout = "> /dev/null"; 
	std::string launch_process_command = "ssh -T " + machine_ssh + " \"cd cs-425-mp-1 && " + 
		(test_mode ? testing_mode_compile_program : compile_program) + 
		" && ./server_client " + std::to_string(machine_number) + " " + space_seperate_machine_names + ignore_stdout + "\"" + ignore_stdout;
	char* heap_ptr = (char*)malloc(launch_process_command.length() + 1);
	memcpy(heap_ptr, launch_process_command.c_str(), launch_process_command.length() + 1);

	processes.emplace_back();
	pthread_create(&processes.back(), NULL, exec_command, heap_ptr);
}

void wait_for_all_process_to_start(const std::vector<int>& machines, 
		int querier_stdin_pipe, int querier_stdout_pipe,
		std::unordered_set<int> skip_machines = {} 
		) {
	while (true) {
		usleep(2000000); // sleep for 2 seconds
		// send the grep command to our querier using its stdin pipe
		std::string command = "grep \".*\"\n";
		write(querier_stdin_pipe, command.c_str(), command.length());

		// here's the expected output from the querier
		std::vector<regex_wrapper> expected_strings;
		for (size_t i = 0; i < machines.size(); ++i) {
			if (skip_machines.find(i) != skip_machines.end()) {
				continue;
			}
			int machine_number = machines[i];
			std::string machine_name = get_machine_name(machine_number);
#ifndef PRINT_ALL_LINES
			std::string regex_str = "vm" + std::to_string(machine_number) + ": .*";
			expected_strings.push_back(regex_wrapper(regex_str));
#else
			for (int i = 0; i < num_files_per_machine; ++i) {
				std::string regex_str = machine_name + std::string(".") + std::to_string(i) + std::string(".log.*");
				expected_strings.push_back(regex_wrapper(regex_str));
			}
#endif
		}
#ifndef PRINT_ALL_LINES
		expected_strings.push_back(regex_wrapper("total: .*"));
#endif

		// validate querier machines by reading its output from stdout pipe
		bool success = client_reply_matches(expected_strings, querier_stdout_pipe);
		if (success) {
			break;
		}
	}
}

void start_process_on_distributed(const std::vector<int>& machines, const std::string& space_seperate_machine_names, 
		std::unordered_set<int> skip_machines = {}, std::unordered_set<int> machines_run_in_test_mode = {}) {
	bool run_all_machines_in_test_mode = machines_run_in_test_mode.find(-1) != machines_run_in_test_mode.end();
	assert(processes.size() == 0 && num_processes_running == 0);
	for (size_t i = 0; i < machines.size(); ++i) {
		if (skip_machines.find(i) != skip_machines.end()) {
			continue;
		}
		std::cout << "starting" << ((run_all_machines_in_test_mode || (machines_run_in_test_mode.find(i) != machines_run_in_test_mode.end())) ? " test " : " ") << "process on machine " << machines[i] << std::endl;
		start_process(machines[i], space_seperate_machine_names, 
				run_all_machines_in_test_mode || (machines_run_in_test_mode.find(i) != machines_run_in_test_mode.end()));
	}
	while (num_processes_running < processes.size()) {
		std::cout << "waiting for " << (processes.size() - num_processes_running) << " processes to start..." << std::endl;
		usleep(2000000); // sleep for 2 seconds
	}
	usleep(3000000); // give all servers a second to setup
}

void terminate_process_on_distributed(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::cout << "killng machine " << machine_number << std::endl;
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = user + "@" + machine_name;
		std::string terminate_process_command = "ssh " + machine_ssh + " pkill server_client &> /dev/null";
		system(terminate_process_command.c_str());
	}
	assert(processes.size() == num_processes_running);
	for (const pthread_t& thread : processes) {
		pthread_join(thread, NULL);
		--num_processes_running;
	}
	processes.clear();
}

std::vector<char*> split_string(const std::string& str_to_split) {
	std::vector<char*> vec;
	std::stringstream ss(str_to_split);
	std::string substring;
	while (getline(ss, substring, ' ')) {
		char* char_str = new char[substring.size() + 1];
		std::strcpy(char_str, substring.c_str());
		vec.push_back(char_str);
	}
	return vec;
}

void run_querier(const std::string& space_seperate_machine_names, int machine_number, 
		int* stdin_pipe_out, int* stdout_pipe_out, int* child_pid_out, bool test_mode = false) {
	std::cout << "running querier" << std::endl;
	std::string machine_name = get_machine_name(machine_number);
	compile_process(machine_name, test_mode);

	std::vector<char*> argv_vec = split_string(space_seperate_machine_names);
	int extra = 7;
	char** argv = (char**)malloc(sizeof(char*) * (argv_vec.size() + extra));

	argv[0] = "ssh";
	std::string machine_ssh = user + "@" + machine_name;
	argv[1] = const_cast<char*>(machine_ssh.c_str());
	argv[2] = "cd cs-425-mp-1";
	argv[3] = "&&";
	argv[4] = "./server_client";
	argv[5] = const_cast<char*>(std::to_string(machine_number).c_str());
	argv[argv_vec.size() + extra - 1] = NULL;

	for (size_t i = 0; i < argv_vec.size(); ++i) {
		argv[i + extra - 1] = argv_vec[i];
	}

	int stdout_pipe[2];
	int stdin_pipe[2];
	if (pipe(stdout_pipe) < 0 || pipe(stdin_pipe) < 0) {
		std::cerr << "could not open pipe" << std::endl;
		return;
	}

	int child_pid = fork();
	if (child_pid < 0) {
		std::cerr << "could not fork" << std::endl;
	} else if (child_pid == 0) {
		close(stdout_pipe[0]);
		close(stdin_pipe[1]);

		dup2(stdout_pipe[1], STDOUT_FILENO);
		dup2(stdin_pipe[0], STDIN_FILENO);

		execve("/bin/ssh", argv, NULL);

		perror("exec failed");

		// handle case where exec fails
		close(stdout_pipe[1]);
		close(stdin_pipe[0]);
		exit(1);
	} else {
		close(stdout_pipe[1]);
		close(stdin_pipe[0]);

		*stdout_pipe_out = stdout_pipe[0];
		*stdin_pipe_out = stdin_pipe[1];
		*child_pid_out = child_pid;
	}
}

bool client_reply_matches(const std::vector<regex_wrapper>& expected_replies, int client_output_fd) {
	static std::string prefix = "Enter grep command: ";

	char* reply = NULL;
	size_t reply_length = 0;
	FILE* stdout_stream = fdopen(client_output_fd, "r");

	int nbytes = 0;
	std::vector<bool> output_matched(expected_replies.size(), false); 
	size_t num_outputs_matched = 0;
	while ((nbytes = getline(&reply, &reply_length, stdout_stream)) > 0) {
		if (reply[nbytes - 1] == '\n') {
			reply[nbytes - 1] = '\0';
		}

		std::string server_reply(reply);
		if (server_reply.compare(0, prefix.length(), prefix) == 0) {
			server_reply = server_reply.erase(0, prefix.length());
		}

		if (server_reply == "\x08") { // indicates EOF
			break;
		}

		if (server_reply.length() > 0 && num_outputs_matched == expected_replies.size()) { // program sent more output lines than expected
			std::cout << server_reply << " not expected" << std::endl;
			return false;
		}

		for (size_t i = 0; i < expected_replies.size(); ++i) {
			if (output_matched[i] == true) continue;
			bool match = std::regex_match(server_reply.begin(), server_reply.end(), expected_replies[i].regex);
			if (match) {
				output_matched[i] = true;
				num_outputs_matched += 1;
				break;
			} else {
				//std::cout << server_reply << "!=" << expected_replies[i].regex_str << std::endl;
			}
		}
	}
	return num_outputs_matched == expected_replies.size();
}

void test_infrequent_pattern(const std::vector<int>& machines, int querier_machine_index, const std::string& space_seperate_machine_names, 
		int num_files_per_machine) {
	// run our process on each machine (except the querier machine)
	start_process_on_distributed(machines, space_seperate_machine_names, std::unordered_set<int>({querier_machine_index}));

	// run our process on the querier machine and pipe its stdin and stdout into stdin_pipe and stdout_pipe. Our current machine will
	// be our querier
	int stdin_pipe, stdout_pipe, child_pid;
	run_querier(space_seperate_machine_names, machines[querier_machine_index], &stdin_pipe, &stdout_pipe, &child_pid);

	wait_for_all_process_to_start(machines, stdin_pipe, stdout_pipe);

	// send the grep command to our querier using its stdin pipe
	std::string command = "grep \"ID -\"\n";
	write(stdin_pipe, command.c_str(), command.length());

	// here's the expected output from the querier
	std::vector<regex_wrapper> expected_strings;
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
#ifndef PRINT_ALL_LINES
		std::string regex_str = "vm" + std::to_string(machine_number) + ": " + std::to_string(num_files_per_machine);
		expected_strings.push_back(regex_wrapper(regex_str));	
#else
		for (int i = 0; i < num_files_per_machine; ++i) {
			std::string file_name = machine_name + std::string(".") + std::to_string(i) + std::string(".log");
			std::string regex_str = std::string("test/") + file_name + std::string(":ID - ") + file_name;
			expected_strings.push_back(regex_wrapper(regex_str));
		}
#endif
	}
#ifndef PRINT_ALL_LINES
	expected_strings.push_back(regex_wrapper("total: " + std::to_string(machines.size() * num_files_per_machine)));
#endif

	// validate querier machines by reading its output from stdout pipe
	bool success = client_reply_matches(expected_strings, stdout_pipe);
	std::cout << "test_infrequent_pattern: " << (success ? "Success" : "Failed") << std::endl;
	if (success) {
		++num_success_tests;
	}
	++num_total_tests;

	close(stdin_pipe);
	close(stdout_pipe);
	kill(child_pid, SIGKILL);

	waitpid(child_pid, NULL, 0);

	// terminate our process on each machine
	terminate_process_on_distributed(machines);
}

void test_abrupt_connection_closed(const std::vector<int>& machines, int querier_machine_index, 
		const std::string& space_seperate_machine_names, int num_files_per_machine) {
	// run our process on each machine (except the querier machine)
	start_process_on_distributed(machines, space_seperate_machine_names, std::unordered_set<int>({querier_machine_index}), 
			std::unordered_set<int>({-1}));

	// run our process on the querier machine and pipe its stdin and stdout into stdin_pipe and stdout_pipe. Our current machine will
	// be our querier
	int stdin_pipe, stdout_pipe, child_pid;
	run_querier(space_seperate_machine_names, machines[querier_machine_index], &stdin_pipe, &stdout_pipe, &child_pid, true);

	wait_for_all_process_to_start(machines, stdin_pipe, stdout_pipe);

	// send the grep command to our querier using its stdin pipe
	std::string command = "grep TEST_SERVER_EXIT_UPON_CONNECTION\n";
	write(stdin_pipe, command.c_str(), command.length());

	// here's the expected output from the querier
	std::vector<regex_wrapper> expected_strings = {regex_wrapper("total: 0")};

	// validate querier machines by reading its output from stdout pipe
	bool success = client_reply_matches(expected_strings, stdout_pipe);
	std::cout << "test_abrupt_connection_closed: " << (success ? "Success" : "Failed") << std::endl;
	if (success) {
		++num_success_tests;
	}
	++num_total_tests;

	close(stdin_pipe);
	close(stdout_pipe);
	kill(child_pid, SIGKILL);

	waitpid(child_pid, NULL, 0);

	// terminate our process on each machine
	terminate_process_on_distributed(machines);
}

void test_failed_machine_mid_execution(const std::vector<int>& machines, int querier_machine_index, 
		const std::string& space_seperate_machine_names, int num_files_per_machine, int failed_machine_index) {
	// run our process on each machine (except the querier machine)
	start_process_on_distributed(machines, space_seperate_machine_names, std::unordered_set<int>({querier_machine_index}), 
			std::unordered_set<int>({failed_machine_index}));

	// run our process on the querier machine and pipe its stdin and stdout into stdin_pipe and stdout_pipe. Our current machine will
	// be our querier
	int stdin_pipe, stdout_pipe, child_pid;
	run_querier(space_seperate_machine_names, machines[querier_machine_index], &stdin_pipe, &stdout_pipe, &child_pid);

	wait_for_all_process_to_start(machines, stdin_pipe, stdout_pipe);

	// send the grep command to our querier using its stdin pipe
	std::string command = "grep TEST_SERVER_CRASH_UPON_CONNECTION\n";
	write(stdin_pipe, command.c_str(), command.length());

	// here's the expected output from the querier
	std::vector<regex_wrapper> expected_strings;
	for (const int machine_number : machines) {
		if (machine_number == machines[failed_machine_index]) {
			continue;
		}
		std::string machine_name = get_machine_name(machine_number);
#ifndef PRINT_ALL_LINES
		std::string regex_str =  "vm" + std::to_string(machine_number) + ": " + std::to_string(num_files_per_machine);
		expected_strings.push_back(regex_wrapper(regex_str));	
#else
		for (int i = 0; i < num_files_per_machine; ++i) {
			std::string file_name = machine_name + std::string(".") + std::to_string(i) + std::string(".log");
			std::string regex_str = std::string("test/") + file_name + std::string(":TEST_SERVER_CRASH_UPON_CONNECTION");
			expected_strings.push_back(regex_wrapper(regex_str));
		}
#endif
	}
#ifndef PRINT_ALL_LINES
	expected_strings.push_back(regex_wrapper("total: " + std::to_string((machines.size() - 1) * num_files_per_machine)));
#endif

	// validate querier machines by reading its output from stdout pipe
	bool success = client_reply_matches(expected_strings, stdout_pipe);
	std::cout << "test_failed_machine_mid_execution: " << (success ? "Success" : "Failed") << std::endl;
	if (success) {
		++num_success_tests;
	}
	++num_total_tests;

	close(stdin_pipe);
	close(stdout_pipe);
	kill(child_pid, SIGKILL);

	waitpid(child_pid, NULL, 0);

	// terminate our process on each machine
	terminate_process_on_distributed(machines);
}

void test_machine_unreachable(const std::vector<int>& machines, int querier_machine_index, const std::string& space_seperate_machine_names, 
		int num_files_per_machine, int failed_machine_index) {
	// run our process on each machine (except the querier machine)
	start_process_on_distributed(machines, space_seperate_machine_names, 
			std::unordered_set<int>({querier_machine_index, failed_machine_index}));

	// run our process on the querier machine and pipe its stdin and stdout into stdin_pipe and stdout_pipe. Our current machine will
	// be our querier
	int stdin_pipe, stdout_pipe, child_pid;
	run_querier(space_seperate_machine_names, machines[querier_machine_index], &stdin_pipe, &stdout_pipe, &child_pid);

	wait_for_all_process_to_start(machines, stdin_pipe, stdout_pipe,  std::unordered_set<int>({failed_machine_index}));

	// send the grep command to our querier using its stdin pipe
	std::string command = "grep \"ID -\"\n";
	write(stdin_pipe, command.c_str(), command.length());

	// here's the expected output from the querier
	std::vector<regex_wrapper> expected_strings;
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		if (machine_number == machines[failed_machine_index]) {
			continue;
		}
#ifndef PRINT_ALL_LINES
		std::string regex_str =  "vm" + std::to_string(machine_number) + ": " + std::to_string(num_files_per_machine);
		expected_strings.push_back(regex_wrapper(regex_str));	
#else
		for (int i = 0; i < num_files_per_machine; ++i) {
			std::string file_name = machine_name + std::string(".") + std::to_string(i) + std::string(".log");
			std::string regex_str = std::string("test/") + file_name + std::string(":ID - ") + file_name;
			expected_strings.push_back(regex_wrapper(regex_str));
		}
#endif
	}
#ifndef PRINT_ALL_LINES
	expected_strings.push_back(regex_wrapper("total: " + std::to_string((machines.size() - 1) * num_files_per_machine)));
#endif 
	// validate querier machines by reading its output from stdout pipe
	bool success = client_reply_matches(expected_strings, stdout_pipe);
	std::cout << "test_machine_unreachable: " << (success ? "Success" : "Failed") << std::endl;
	if (success) {
		++num_success_tests;
	}
	++num_total_tests;

	close(stdin_pipe);
	close(stdout_pipe);
	kill(child_pid, SIGKILL);

	waitpid(child_pid, NULL, 0);

	// terminate our process on each machine
	terminate_process_on_distributed(machines);
}

void run_trial(std::string command, int stdin_pipe, int stdout_pipe, int child_pid) {
	// send the grep command to our querier using its stdin pipe
	write(stdin_pipe, command.c_str(), command.length());

	// read through querier output until '\x08' byte is detected
	char* reply = nullptr;
	size_t reply_length = 0;
	FILE* stdout_stream = fdopen(stdout_pipe, "r");
	int nbytes = 0;
	while ((nbytes = getline(&reply, &reply_length, stdout_stream)) > 0) {
		//std::cout << reply;
		if (reply[nbytes - 1] == '\x08' || reply[nbytes - 2] == '\x08') { // indicates EOF
			break;
		}
	}
}

void run_experiment(std::string command, int stdin_pipe, int stdout_pipe, int child_pid, int frequency) {
	int num_trials = 7;
	std::cout << "Starting trials for pattern with " << frequency << "\% frequency\nLatency:" << std::endl;
	for (int i = 0; i < num_trials; ++i) {
		auto start = std::chrono::high_resolution_clock::now();
		run_trial(command, stdin_pipe, stdout_pipe, child_pid);
		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

		size_t latency = duration.count();
		std::cout << latency << std::endl;
	}
}

void run_all_experiments(const std::vector<int>& machines, int querier_machine_index, const std::string& space_seperate_machine_names) {
	setup_provided_logs(machines);
	// run our process on each machine (except the querier machine)
	start_process_on_distributed(machines, space_seperate_machine_names, std::unordered_set<int>({querier_machine_index}));

	// run our process on the querier machine and pipe its stdin and stdout into stdin_pipe and stdout_pipe. Our current machine will
	// be our querier
	int stdin_pipe, stdout_pipe, child_pid;
	run_querier(space_seperate_machine_names, machines[querier_machine_index], &stdin_pipe, &stdout_pipe, &child_pid);

	wait_for_all_process_to_start(machines, stdin_pipe, stdout_pipe);

	run_trial(std::string("grep NON_EXISTENT\n"), stdin_pipe, stdout_pipe, child_pid); // dry run

	std::string command = "grep ABCD\n"; // 0% frequency
	run_experiment(command, stdin_pipe, stdout_pipe, child_pid, 0);

	command = "grep PUT\n"; // 20% frequency
	run_experiment(command, stdin_pipe, stdout_pipe, child_pid, 20);

	command = "grep -v GET\n"; // 40% frequency
	run_experiment(command, stdin_pipe, stdout_pipe, child_pid, 40);

	command = "grep GET\n"; // 60% frequency
	run_experiment(command, stdin_pipe, stdout_pipe, child_pid, 60);

	command = "grep -v PUT\n"; // 80% frequency
	run_experiment(command, stdin_pipe, stdout_pipe, child_pid, 80);

	command = "grep HTTP\n"; // 100% frequency
	run_experiment(command, stdin_pipe, stdout_pipe, child_pid, 100);

	close(stdin_pipe);
	close(stdout_pipe);
	kill(child_pid, SIGKILL);

	waitpid(child_pid, NULL, 0);

	// terminate our process on each machine
	terminate_process_on_distributed(machines);
	delete_provided_logs(machines);
}

void get_grep_results(std::string grep_command, std::vector<regex_wrapper>& expected_strings, std::string server_reply_prefix) {
	FILE* pipe = popen(grep_command.c_str(), "r");
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}

	char* reply = nullptr;
	size_t reply_length = 0;
	size_t count = 0;
	int nbytes = 0;
	while ((nbytes = getline(&reply, &reply_length, pipe)) > 0) {
		if (reply[nbytes - 1] == '\n') {
			reply[nbytes - 1] = '\0';
		}
		std::string reply_str = std::string(reply);
		if (reply_str == "\x08") { // indicates EOF
			break;
		}
		reply_str = server_reply_prefix + reply_str;
		expected_strings.push_back(regex_wrapper(reply_str));
	}
	pclose(pipe);
}

std::vector<regex_wrapper> expected_grep_on_provided_logs(const std::vector<int>& machines, std::string command) {
	std::vector<regex_wrapper> expected_strings;
	std::string all_log_files = "";
	for (const int machine_number : machines) {
		std::string log_file = " provided_logs/vm" + std::to_string(machine_number) + ".log";
		all_log_files += log_file;
		std::string grep_command = "grep " + command + log_file;
		std::string reply_prefix = "";
#ifndef PRINT_ALL_LINES
		grep_command += " -ch | awk 'BEGIN { sum=0 } { sum+=$1 } END {print sum }'";
		reply_prefix = "vm" + std::to_string(machine_number) + ": ";
#endif
		grep_command += "\n";
		get_grep_results(grep_command, expected_strings, reply_prefix);
	}
#ifndef PRINT_ALL_LINES
	std::string grep_command = "grep " + command + all_log_files;
	grep_command += " -ch | awk 'BEGIN { sum=0 } { sum+=$1 } END {print sum }'";
	grep_command += "\n";
	get_grep_results(grep_command, expected_strings, "total: ");
#endif

	return expected_strings;
}

void test_pattern_in_provided_logs(const std::vector<int>& machines, int querier_machine_index, const std::string& space_seperate_machine_names, 
		std::vector<std::string> patterns) {
	setup_provided_logs(machines);
	// run our process on each machine (except the querier machine)
	start_process_on_distributed(machines, space_seperate_machine_names, std::unordered_set<int>({querier_machine_index}));

	// run our process on the querier machine and pipe its stdin and stdout into stdin_pipe and stdout_pipe. Our current machine will
	// be our querier
	int stdin_pipe, stdout_pipe, child_pid;
	run_querier(space_seperate_machine_names, machines[querier_machine_index], &stdin_pipe, &stdout_pipe, &child_pid);

	wait_for_all_process_to_start(machines, stdin_pipe, stdout_pipe);

	for (const std::string& pattern : patterns) {
		// send the grep command to our querier using its stdin pipe
		std::string command = "grep " + pattern + "\n";
		write(stdin_pipe, command.c_str(), command.length());

		// here's the expected output from the querier
		std::vector<regex_wrapper> expected_strings = expected_grep_on_provided_logs(machines, pattern);
		//
		// validate querier machines by reading its output from stdout pipe
		bool success = client_reply_matches(expected_strings, stdout_pipe);
		std::cout << "test_pattern_in_provided_logs pattern " << pattern << ": " << (success ? "Success" : "Failed") << std::endl;
		if (success) {
			++num_success_tests;
		}
		++num_total_tests;
	}

	close(stdin_pipe);
	close(stdout_pipe);
	kill(child_pid, SIGKILL);

	waitpid(child_pid, NULL, 0);

	// terminate our process on each machine
	terminate_process_on_distributed(machines);
	delete_provided_logs(machines);
}

int random_index(int max_num) {
	srand((unsigned) time(NULL));
	int random = rand() % max_num;
	return random;
}

void test_machine_reachable(int querier_machine_index, const std::string& space_seperate_machine_names, 
		int num_files_per_machine){
	//generate random machine vector of 3
	std::vector<int> machines;
	for (int i = 0; i < 3; i++){
		int index = random_index(10);
		machines.push_back(index);
	}

	// run our process on random machine (except the querier machine)
	start_process_on_distributed(machines, space_seperate_machine_names, std::unordered_set<int>({querier_machine_index}), 
			std::unordered_set<int>({-1}));

	// run our process on the querier machine and pipe its stdin and stdout into stdin_pipe and stdout_pipe. 
	int stdin_pipe, stdout_pipe, child_pid;

	run_querier(space_seperate_machine_names, machines[querier_machine_index], &stdin_pipe, &stdout_pipe, &child_pid);

	wait_for_all_process_to_start(machines, stdin_pipe, stdout_pipe);

	// send the grep command to our querier using its stdin pipe
	std::string command = "grep ID\n";
	write(stdin_pipe, command.c_str(), command.length());

	// here's the expected output from the querier
	std::vector<regex_wrapper> expected_strings;
	for (const int machine_number : machines) {
#ifndef PRINT_ALL_LINES
		std::string regex_str =  "vm" + std::to_string(machine_number) + ": " + std::to_string(num_files_per_machine);
		expected_strings.push_back(regex_wrapper(regex_str));	
#else
		std::string machine_name = get_machine_name(machine_number);
		for (int i = 0; i < num_files_per_machine; ++i) {
			std::string file_name = machine_name + std::string(".") + std::to_string(i) + std::string(".log");
			std::string regex_str = std::string("test/") + file_name + std::string(":ID - ") + file_name;
			expected_strings.push_back(regex_wrapper(regex_str));
		}
#endif
	}
#ifndef PRINT_ALL_LINES
	expected_strings.push_back(regex_wrapper("total: " + std::to_string(machines.size() * num_files_per_machine)));
#endif
	bool success = client_reply_matches(expected_strings, stdout_pipe);
	std::cout << "test_machine_reachable: " << (success ? "Success" : "Failed") << std::endl;
	if (success) {
		++num_success_tests;
	}
	++num_total_tests;

	close(stdin_pipe);
	close(stdout_pipe);
	kill(child_pid, SIGKILL);

	waitpid(child_pid, NULL, 0);

	// terminate our process on each machine
	terminate_process_on_distributed(machines);
}

void tests_with_generated_logs(std::vector<int>& machines, std::string space_seperate_machine_names, int num_files) {
	if (should_generate_log_files) {
		setup_generated_logs(machines, num_files);
	}

	test_infrequent_pattern(machines, 0, space_seperate_machine_names, num_files);
	test_abrupt_connection_closed(machines, 0, space_seperate_machine_names, num_files);
	test_machine_unreachable(machines, 0, space_seperate_machine_names, num_files, 1);
	test_failed_machine_mid_execution(machines, 0, space_seperate_machine_names, num_files, 1);
	//test_machine_reachable(0, space_seperate_machine_names,num_files);

	if (should_remove_log_files) {
		delete_generated_logs(machines);
	}
}

int main() {
	//	test_machine_ip_generation();
	//	test_client_server_connection();

	int num_files = 5;

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
	if (should_remove_log_files) {
		teardown_distributed_test(machines);
	}
	std::string space_seperate_machine_names = setup_distributed_test(machines, num_files);

	tests_with_generated_logs(machines, space_seperate_machine_names, num_files);

	test_pattern_in_provided_logs(machines, 0, space_seperate_machine_names, std::vector<std::string>({
				"NON_EXISTENT",
				"ABCD",
				"\"GET.*Mozilla/5.0.*Macintosh\"",
				"\"GET.*Mozilla/5.0\"",
				"-v \"GET.*Mozilla/5.0.*Macintosh\"",
				"\"[.* -[0-9]*].*\""
				"\".*\""
				"GET",
				"-v \"GET\"",
				"PUT",
				"-v \"PUT\"",
				"HTTP"
				}));

	std::cout << "Number of tests passed: " << num_success_tests << "/" << num_total_tests << std::endl;
	while (machines.size() > 4) {
		machines.pop_back();
	}
	std::cout << "Running experiment with following machines: ";
	for (const int machine_number : machines) {
		std::cout << machine_number << " ";
	}
	std::cout << std::endl;
	run_all_experiments(machines, 0, space_seperate_machine_names);

	teardown_distributed_test(machines);
}
