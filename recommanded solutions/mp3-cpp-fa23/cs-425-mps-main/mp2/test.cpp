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
#include "membership_list_manager.hpp"

// extern variables
MembershipListManager membership_list_manager;
int generation_count;
std::mutex protocol_mutex;
int server_port = -1;
enum membership_protocol_options membership_protocol;   
bool end_session = false;
std::string machine_id = "";
std::vector<std::string> machine_ips;

// variables for testing
std::string github_user;
std::string github_token;
std::string mp_directory = "cs-425";
std::string mp2_directory = mp_directory + "/mp2";
std::string mp2_logs_directory = mp2_directory + "/test";

int num_success_tests = 0;
int num_total_tests = 0;

class regex_wrapper {
	public:
		std::string regex_str;
		std::regex regex;

		regex_wrapper(std::string str) : regex_str(str), regex(std::regex(str)) {}
};

// GENERAL HELPERS

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

std::string test_mode_compilation = "-D TESTING";
std::string process_name = "membership_manager";
std::string compile_program = "g++ -std=c++17 -g2 gossip_client.cpp gossip_server.cpp introducer.cpp main.cpp mp1_wrapper.cpp utils.cpp ../mp1/client.cpp ../mp1/server.cpp ../mp1/utils.cpp -I ../mp1 -lpthread -o " + process_name;
std::string testing_mode_compile_program = compile_program + " " + test_mode_compilation;
std::vector<pthread_t> processes;
int num_processes_running = 0;

// Called by pthread_create, uses system to execute command. 
void* exec_command(void* command) {
        char* s = (char*)(command);
        num_processes_running += 1;
        system(s);
        return NULL;
}

// Compiles mp2 on machine identified by machine_name
void compile_process(const std::string machine_name, bool test_mode = false) {
        std::string machine_ssh = github_user + "@" + machine_name;
        std::string launch_process_command = "ssh " + machine_ssh + " \"cd " + mp2_directory + " && " +
                (test_mode ? testing_mode_compile_program : compile_program) + "\"";
	std::cout << "Running: " << launch_process_command << std::endl;
        system(launch_process_command.c_str());
}

// Compiles and runs mp2 on machine identified by machine_number
// mp2 takes introducer number and list of all machine names (space separated) as input. Thus, the function takes introducer_number and 
// space_seperate_machine_names and passes them as input when it runs mp2 
// Lanches a new thread to run mp2; stores the thread into processes vector
void start_process(const int machine_number, const int introducer_number, const std::string& space_seperate_machine_names, bool test_mode = false,
			const int message_drop_rate = 0) {
        std::string machine_name = get_machine_name(machine_number);
        std::string machine_ssh = github_user + "@" + machine_name;
        std::string ignore_stdout = "> /dev/null";
        std::string launch_process_command = "ssh -T " + machine_ssh + " \"" + 
		"cd " + mp2_directory + " && " + // cd into mp2
                (test_mode ? testing_mode_compile_program : compile_program) + " && " +  // compile mp2
		// run mp2
		"./" + process_name + " " + std::to_string(machine_number) + " 0 " + std::to_string(introducer_number) + " " 
			+ std::to_string(message_drop_rate) + " " + space_seperate_machine_names + ignore_stdout + "\"" 
		+ ignore_stdout;
	std::cout << "running: "  << launch_process_command << std::endl;
        char* heap_ptr = (char*)malloc(launch_process_command.length() + 1);
        memcpy(heap_ptr, launch_process_command.c_str(), launch_process_command.length() + 1);

        processes.emplace_back();
        pthread_create(&processes.back(), NULL, exec_command, heap_ptr);
}

// Runs grep_command and returns the grep output
std::string get_grep_results(std::string grep_command) {
	FILE* pipe = popen(grep_command.c_str(), "r");
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}

	std::string grep_output = "";

	char* reply = nullptr;
	size_t reply_length = 0;
	size_t count = 0;
	int nbytes = 0;
	while ((nbytes = getline(&reply, &reply_length, pipe)) > 0) {
		if (reply[nbytes - 1] == '\n') {
			reply[nbytes - 1] = '\0';
		}
		std::string reply_str = std::string(reply);
		grep_output += reply_str;
	}
	pclose(pipe);
	return grep_output;
}


// Waits until all gossip threads (i.e. gossip server, gossip client, and if needed introducer) are running on machine identified by 
// machine_number.
// When each of these threads starts, it logs "STARTING ..." in the log files and thus, this function calls "grep STARTING" on 
// log files and returns only when grep returns 2 (or 3 in case introducer is also running on machine) 
void wait_for_machine(const int machine_number, bool machine_running_introducer) {
	std::string machine_name = get_machine_name(machine_number);
	std::string machine_ssh = github_user + "@" + machine_name;
	std::string grep_command = "ssh " + machine_ssh + " grep STARTING " + mp2_logs_directory + "/*" + 
		" -ch | awk 'BEGIN { sum=0 } { sum+=$1 } END {print sum }'";
	
	while (true) {
		std::string grep_output = get_grep_results(grep_command);
		if ((not machine_running_introducer && grep_output == "3") || (machine_running_introducer && grep_output == "4")) {
			return;
		}
		usleep(2000000); // sleep for 2 seconds
	}
}

// Compiles and runs mp2 on all machines except ones in skip_machines
// introducer_number and space_seperate_machine_names are inputs provided to mp2 when it is run
void start_process_on_all_machines(const std::vector<int>& machines, const int introducer_index, 
		const std::string& space_seperate_machine_names, std::unordered_set<int> skip_machines = {}, 
		std::unordered_set<int> machines_run_in_test_mode = {}) {
	int introducer_number = machines[introducer_index];
	bool run_all_machines_in_test_mode = machines_run_in_test_mode.find(-1) != machines_run_in_test_mode.end();
        assert(processes.size() == 0 && num_processes_running == 0);

	// Start the introducer first
        start_process(introducer_number, introducer_number, space_seperate_machine_names, 
			run_all_machines_in_test_mode || (machines_run_in_test_mode.find(introducer_index) != machines_run_in_test_mode.end()));
	while (num_processes_running < processes.size()) {
		usleep(2000000); // sleep for 2 seconds
	}
	wait_for_machine(introducer_number, true);

	// Start process on all machines execpt the introducer and machines in skip_machines
	for (size_t i = 0; i < machines.size(); ++i) {
		if (machines[i] == introducer_number || skip_machines.find(i) != skip_machines.end()) {
			continue;
		}
		start_process(machines[i], introducer_number, space_seperate_machine_names,
				run_all_machines_in_test_mode || (machines_run_in_test_mode.find(i) != machines_run_in_test_mode.end()));
	}
	while (num_processes_running < processes.size()) {
		usleep(2000000); // sleep for 2 seconds
	}
	usleep(3000000); // give all servers a second to setup
}

// For every machine in machines, kills all processes with name membership_manager (i.e. process_name). Joins all threads in processes as
// well
void terminate_process_on_all_machines(const std::vector<int>& machines) {
	for (const int machine_number : machines) {
		std::string machine_name = get_machine_name(machine_number);
		std::string machine_ssh = github_user + "@" + machine_name;
		std::string kill_command = "ps -ef | grep " + process_name + " | grep -v test | awk '{print $2}' | xargs kill -9";
		std::string terminate_process_command = "ssh " + machine_ssh + " " + kill_command;
		system(terminate_process_command.c_str());
	}
	assert(processes.size() == num_processes_running);
	for (const pthread_t& thread : processes) {
		pthread_join(thread, NULL);
		--num_processes_running;
	}
	processes.clear();
}

// Splits str_to_split on space characters. Returns a vector all the split string
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

// Runs mp2 on machine_number using fork-exec pattern and redirects the stdin and out into pipes. Returns the pipe fd for child processes
// stdin and stdout using input arguments stdin_pipe_out and stdout_pipe_out, respectively. Returns the PID of the child by argument child_pid_out.
void run_querier(const std::string& space_seperate_machine_names,  int introducer_number, int machine_number, 
		int* stdin_pipe_out, int* stdout_pipe_out, int* child_pid_out, bool test_mode = false, const int message_drop_rate = 0) {
	std::cout << "running querier" << std::endl;
	std::string machine_name = get_machine_name(machine_number);
	compile_process(machine_name, test_mode);

	std::vector<char*> argv_vec = split_string(space_seperate_machine_names);
	int extra = 10;
	char** argv = (char**)malloc(sizeof(char*) * (argv_vec.size() + extra));

	argv[0] = "ssh";
	std::string machine_ssh = github_user + "@" + machine_name;
	std::string cd_command = "cd " + mp2_directory;
	std::string process_name_command = "./" + process_name;
	std::string machine_number_str = std::to_string(machine_number);
	std::string introducer_number_str = std::to_string(introducer_number);
	std::string message_drop_rate_str = std::to_string(message_drop_rate);
	argv[1] = const_cast<char*>(machine_ssh.c_str());
	argv[2] = const_cast<char*>(cd_command.c_str());
	argv[3] = "&&";
	argv[4] = const_cast<char*>(process_name_command.c_str());
	argv[5] = const_cast<char*>(machine_number_str.c_str());
	argv[6] = "0"; // version number
	argv[7] = const_cast<char*>(introducer_number_str.c_str());
	argv[8] = const_cast<char*>(message_drop_rate_str.c_str()); 
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
		std::cerr << "Exec failed" << std::endl;

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

// Reads from client_output_fd and verifies if the client output matches the regex value provided in expected_replies. Returns true
// if it finds a match for all regex values in expected_replies. Otherwise, returns false.
bool client_reply_matches(const std::vector<regex_wrapper>& expected_replies, int client_output_fd) {
	static std::string prefix = "Enter grep command or failure detection protocol: ";

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

		std::cout << "client: " << server_reply << std::endl;

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

// Issues grep commands on querier_stdin_pipe and returns when it gets grep output from every machine in machines that is not in skip_machines
void wait_for_all_process_to_start(const std::vector<int>& machines,
                int querier_stdin_pipe, int querier_stdout_pipe,
                std::unordered_set<int> skip_machines = {}
                ) {
        while (true) {
                usleep(2000000); // sleep for 2 seconds
                // send the grep command to our querier using its stdin pipe
                std::string command = "grep \"STARTING\"\n";
                write(querier_stdin_pipe, command.c_str(), command.length());

                // here's the expected output from the querier
                std::vector<regex_wrapper> expected_strings;
                for (size_t i = 0; i < machines.size(); ++i) {
                        if (skip_machines.find(i) != skip_machines.end()) {
                                continue;
                        }
                        int machine_number = machines[i];
                        std::string machine_name = get_machine_name(machine_number);
                        std::string regex_str = "vm" + std::to_string(machine_number) + ": (3|4)";
                        expected_strings.push_back(regex_wrapper(regex_str));
                }
                expected_strings.push_back(regex_wrapper("total: .*"));

                // validate querier machines by reading its output from stdout pipe
                bool success = client_reply_matches(expected_strings, querier_stdout_pipe);
                if (success) {
                        break;
                }
        }
}
void test_failed_machine_mid_execution(const std::vector<int>& machines, int querier_machine_index, int introducer_machine_index,
		const std::string& space_seperate_machine_names, int failed_machine_index){
	//run process on each machine
	start_process_on_all_machines(machines, introducer_machine_index, space_seperate_machine_names, 
			std::unordered_set<int>({querier_machine_index}));

	// run our process on the querier machine and pipe its stdin and stdout into stdin_pipe and stdout_pipe. Our current machine will
	// be our querier
	int stdin_pipe, stdout_pipe, child_pid;
	run_querier(space_seperate_machine_names, machines[introducer_machine_index], machines[querier_machine_index], 
			&stdin_pipe, &stdout_pipe, &child_pid);

	wait_for_all_process_to_start(machines, stdin_pipe, stdout_pipe);
	std::string command = "GOSSIP\n";
    write(stdin_pipe, command.c_str(), command.length());
	// After sending the GOSSIP command, wait for 5 seconds
	std::this_thread::sleep_for(std::chrono::seconds(5)); 

	// Retrieve the membership
	const std::vector<MemberInformation>& membership_list = membership_list_manager.get_membership_list();

	bool success = false;
	// Check the membership list to see if the failure was detected
	for(const MemberInformation& member : membership_list){
		if(member.get_machine_id() == failed_machine_index && member.machine_state_to_string() == "leave") {
			success = true;
			break;
		}
	}

// If the failure was detected, the test is a success

std::cout << "test_failed_machine_mid_execution: " << (success ? "Success" : "Failed") << std::endl;

	if (success) {
		++num_success_tests;
	}
	++num_total_tests;

	close(stdin_pipe);
	close(stdout_pipe);
	kill(child_pid, SIGKILL);



	}
void test_everything_starts(const std::vector<int>& machines, int querier_machine_index, int introducer_machine_index,
		const std::string& space_seperate_machine_names) {
	assert(querier_machine_index != introducer_machine_index);
	// run our process on each machine (except the querier machine)
	start_process_on_all_machines(machines, introducer_machine_index, space_seperate_machine_names, 
			std::unordered_set<int>({querier_machine_index}));

	// run our process on the querier machine and pipe its stdin and stdout into stdin_pipe and stdout_pipe. Our current machine will
	// be our querier
	int stdin_pipe, stdout_pipe, child_pid;
	run_querier(space_seperate_machine_names, machines[introducer_machine_index], machines[querier_machine_index], 
			&stdin_pipe, &stdout_pipe, &child_pid);

	wait_for_all_process_to_start(machines, stdin_pipe, stdout_pipe);

	close(stdin_pipe);
	close(stdout_pipe);
	kill(child_pid, SIGKILL);

	waitpid(child_pid, NULL, 0);

	// terminate our process on each machine
	terminate_process_on_all_machines(machines);

	// Nothing crashed! Test passes
	std::cout << "test_everything_starts: " << "Success" << std::endl;
	++num_success_tests;
	++num_total_tests;
}

int main() {
	github_user = getenv("GITHUB_USERNAME");
	github_token = getenv("GITHUB_TOKEN");
	std::cout << "Running tests under " << github_user << " with token " << github_token << std::endl;

	std::vector<int> machines;
	machines.push_back(1);
	machines.push_back(2);

	terminate_process_on_all_machines(machines);
	delete_logs(machines);

	std::string space_seperate_machine_names = get_space_seperated_machine_names(machines);
	pull_from_repo_on_all_machines(machines);
	setup_logs(machines);

	test_everything_starts(machines, 0, 1, space_seperate_machine_names);

	std::cout << "Number of tests passed: " << num_success_tests << "/" << num_total_tests << std::endl;

	delete_logs(machines);

	return 0;
}
