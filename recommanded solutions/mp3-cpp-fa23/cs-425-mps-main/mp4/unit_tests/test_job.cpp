#include <arpa/inet.h>
#include <sys/stat.h>

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <string>
#include <iostream>

#include "job.hpp"
#include "utils.hpp"
#include "../mp3/utils.hpp"
#include "../mp3/unit_tests/test_includes.hpp"

static const std::string executable = "exe.exe";
static const std::string input_file = "job_input.txt";
static const std::string output_file = "job_output.txt";
static const JobType job_type = JobType::MAPLE;
static const uint32_t job_id = 1032;
static const uint32_t num_tasks = 4;
uint32_t self_machine_number;

Job init_job(uint32_t& serialzied_size) {
	serialzied_size = (5 * sizeof(uint32_t)) + executable.length() + input_file.length() + output_file.length();
	return Job(job_id, job_type, num_tasks, executable, input_file, output_file);
}

void test_job_serialization() {
	uint32_t expected_serialized_size;
	Job job = init_job(expected_serialized_size);
	assert(job.struct_serialized_size() == expected_serialized_size);

	int server_socket = setup_tcp_server_socket(2023);                                                                                        
	int client_socket = get_tcp_socket_with_node(self_machine_number, 2023);                                                                  
	int client_fd = accept(server_socket, NULL, NULL);                                                                                        
	assert(job.serialize(client_socket) == expected_serialized_size + sizeof(uint32_t));

	Job deserialized = Job::deserialize(client_fd, "TEST_JOB");
	deserialized.set_id(job.get_id());
	assert(job == deserialized);

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
	test_job_serialization();
	return 0;
}
