#pragma once 
#include <arpa/inet.h>

#include <cassert>
#include <functional>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <queue>
#include <thread>

#include "job_type.hpp"
#include "utils.hpp"

#include "../mp1/utils.hpp"
#include "../mp2/utils.hpp"

class Job {
private:
	uint32_t job_id;
	JobType job_type;
	uint32_t num_tasks;
	std::string executable_file_name;
	std::string input_file; 				// for MAPLE, the directory containing input files and for JUICE, the prefix of the intermediate files to use as input
	std::string output_file; 				// for MAPLE, the prefix of the intermediate files to use as output and for JUICE, the output file

public:
	Job() : job_id(0), job_type(JobType::INVALID) {}

	Job(uint32_t job_id, JobType job_type, uint32_t num_tasks, std::string executable_file_name, std::string input_file, std::string output_file) :
		job_id(job_id), job_type(job_type), num_tasks(num_tasks), executable_file_name(executable_file_name), input_file(input_file), output_file(output_file) {}

	Job(const Job& job) = delete;
	Job& operator=(const Job& job) = delete;

	Job(Job&& job) : job_id(job.job_id), job_type(job.job_type), num_tasks(job.num_tasks), executable_file_name(job.executable_file_name), 
		input_file(job.input_file), output_file(job.output_file) {}

	Job& operator=(Job&& job) {
		this->job_id = job.job_id;
		this->job_type = job.job_type;
		this->num_tasks = job.num_tasks;
		this->executable_file_name = job.executable_file_name;
		this->input_file = job.input_file;
		this->output_file = job.output_file;

		return *this;
	}

	bool operator==(const Job& job) const {
		return this->job_id == job.job_id &&
				this->job_type == job.job_type &&
				this->num_tasks == job.num_tasks &&
				this->executable_file_name == job.executable_file_name &&
				this->input_file == job.input_file &&
				this->output_file == job.output_file;
	}

	JobType get_job_type() const {
		return job_type;
	}

	uint32_t get_id() const {
		return job_id;
	}

	void set_id(uint32_t id) {
		job_id = id;
	}
	std::string get_input_file() const{
		return input_file;
	}
	// Returns size of buffer required to serliaze this.
	// The structure of the char buffer for each job type is defined in lucid chart:
	// https://lucid.app/lucidchart/e997188c-bcdf-40c2-84b4-d836f0ceb22e/edit?page=0_0&invitationId=inv_d71841b7-1540-48f6-9b7e-ac7f7bbcfb97#
	uint32_t struct_serialized_size() const {
		return sizeof(uint32_t) + sizeof(uint32_t) + // job_type & num_tasks
			sizeof(uint32_t) + executable_file_name.length() + // length of executable and executable name
			sizeof(uint32_t) + input_file.length() + // length of input file and input file
			sizeof(uint32_t) + output_file.length(); // length of output file and output file
	}

	// Serializes fields of this and sends it over the socket.
	// The structure of the char buffer for each job type is defined in lucid chart:
	// https://lucid.app/lucidchart/e997188c-bcdf-40c2-84b4-d836f0ceb22e/edit?page=0_0&invitationId=inv_d71841b7-1540-48f6-9b7e-ac7f7bbcfb97#
	uint32_t serialize(int socket) const {
		uint32_t packet_size = struct_serialized_size() + sizeof(uint32_t); // size of footer
		uint32_t network_packet_size = htonl(packet_size);
		send(socket, (char*)&network_packet_size, sizeof(uint32_t), 0);

		uint32_t network_job_type = htonl(static_cast<uint32_t>(job_type));
		send(socket, (char*)&network_job_type, sizeof(uint32_t), 0);

		uint32_t network_num_tasks = htonl(num_tasks);
		send(socket, (char*)&network_num_tasks, sizeof(uint32_t), 0);
	
		uint32_t network_exe_file_name_len = htonl(executable_file_name.length());
		send(socket, (char*)&network_exe_file_name_len, sizeof(uint32_t), 0);
		send(socket, executable_file_name.c_str(), executable_file_name.length(), 0);
	
		uint32_t network_input_file_name_len = htonl(input_file.length());
		send(socket, (char*)&network_input_file_name_len, sizeof(uint32_t), 0);
		send(socket, input_file.c_str(), input_file.length(), 0);

		uint32_t network_output_file_name_len = htonl(output_file.length());
		send(socket, (char*)&network_output_file_name_len, sizeof(uint32_t), 0);
		send(socket, output_file.c_str(), output_file.length(), 0);
		
		uint32_t network_footer = htonl(FOOTER);
		send(socket, (char*)&network_footer, sizeof(uint32_t), 0);
		shutdown(socket, SHUT_WR);

		return packet_size;
	}

	// Initialies this using the contents of the buffer. Returns the number of bytes from buffer is used when initializing this.
	int32_t deserialize_self(int socket, std::string print_statement_origin) {
		uint32_t packet_size;
		if (read_from_socket(socket, (char*)&packet_size, sizeof(uint32_t)) != sizeof(uint32_t)) {
			std::cout << print_statement_origin << "Client did not include packet size in the message" << std::endl;
			return -1;
        	}
		packet_size = ntohl(packet_size);

		char* buffer_start = new char[packet_size];
		char* buffer = buffer_start;
		if (read_from_socket(socket, buffer_start, packet_size) != packet_size) {
			std::cout << print_statement_origin << "Client did not include job in the message" << std::endl;
			return -1;
        	}

		{
			uint32_t network_job_type;
			memcpy((char*)&network_job_type, buffer, sizeof(uint32_t));
			this->job_type = static_cast<JobType>(ntohl(network_job_type));
			buffer += sizeof(uint32_t);
		}

		{
			uint32_t network_num_tasks;
			memcpy((char*)&network_num_tasks, buffer, sizeof(uint32_t));
			this->num_tasks = ntohl(network_num_tasks);
			buffer += sizeof(uint32_t);
		}

		{
			uint32_t exe_file_name_len;
			memcpy((char*)&exe_file_name_len, buffer, sizeof(uint32_t));
			exe_file_name_len = ntohl(exe_file_name_len);
			buffer += sizeof(uint32_t);

			char* exe_file_name_buffer = new char[exe_file_name_len + 1];
			exe_file_name_buffer[exe_file_name_len] = '\0';
			memcpy(exe_file_name_buffer, buffer, exe_file_name_len);
			this->executable_file_name = std::string(exe_file_name_buffer);
			delete[] exe_file_name_buffer;
			buffer += exe_file_name_len;
		}

		{
			uint32_t input_file_name_len;
			memcpy((char*)&input_file_name_len, buffer, sizeof(uint32_t));
			input_file_name_len = ntohl(input_file_name_len);
			buffer += sizeof(uint32_t);

			char* input_file_name_buffer = new char[input_file_name_len + 1];
			input_file_name_buffer[input_file_name_len] = '\0';
			memcpy(input_file_name_buffer, buffer, input_file_name_len);
			this->input_file = std::string(input_file_name_buffer);
			delete[] input_file_name_buffer;
			buffer += input_file_name_len;
		}

		{
			uint32_t output_file_name_len;
			memcpy((char*)&output_file_name_len, buffer, sizeof(uint32_t));
			output_file_name_len = ntohl(output_file_name_len);
			buffer += sizeof(uint32_t);

			char* output_file_name_buffer = new char[output_file_name_len + 1];
			output_file_name_buffer[output_file_name_len] = '\0';
			memcpy(output_file_name_buffer, buffer, output_file_name_len);
			this->output_file = std::string(output_file_name_buffer);
			delete[] output_file_name_buffer;
			buffer += output_file_name_len;
		}

		uint32_t network_fotter;
		memcpy((char*)&network_fotter, buffer, sizeof(uint32_t));
		assert(ntohl(network_fotter) == FOOTER);
		buffer += sizeof(uint32_t);

		assert(packet_size == (buffer - buffer_start));
		return packet_size;
	}

	static Job deserialize(int socket, std::string print_statement_origin) {
		Job job;
		job.deserialize_self(socket, print_statement_origin);
		return job;
	}

	std::string job_to_string(JobType job) {
	switch(job) {
		case JobType::MAPLE:
			return "Maple";
		case JobType::JUICE:
			return "Juice";
		default:
			return "Invalid";
	}
}
};
