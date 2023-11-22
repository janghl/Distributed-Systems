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

class Task {
private:
	uint32_t task_id;
	JobType task_type;
	std::string executable_file_name;

	// Define the list of inputs
	// For job type MAPLE, inputs is a list of file names and input_range_start/input_range_end define the range of line numbers MAPLE should process
	// For job type JUICE, inputs is a list of keys; JUICE will process files with the name '[input_file_prefix][key]'. JUICE tasks ignore 
	// input_range_start/input_range_end
	std::vector<std::string> inputs;
	std::string input_file_prefix; 				// for job type JUICE, the prefix of the intermediate file
	std::vector<uint32_t> input_range_start;	// for job type MAPLE, the starting line in each input it needs to process
	std::vector<uint32_t> input_range_end;		// for job type MAPLE, the last line in each input it needs to process

public:
	Task() : task_id(0), task_type(JobType::INVALID) {}
	
	// Constructor for MAPLE tasks
	Task(uint32_t task_id, JobType task_type, std::string executable_file_name, std::vector<std::string> inputs,
		std::vector<uint32_t> input_range_start, std::vector<uint32_t> input_range_end) : task_id(task_id), task_type(task_type), 
			executable_file_name(executable_file_name), inputs(inputs), input_range_start(input_range_start), input_range_end(input_range_end) {
		assert(task_type == JobType::MAPLE);
	}

	// Constructor for JUICE tasks
	Task(uint32_t task_id, JobType task_type, std::string executable_file_name, std::vector<std::string> inputs,
		std::string input_file_prefix) : 
			task_id(task_id), task_type(task_type), executable_file_name(executable_file_name), inputs(inputs), input_file_prefix(input_file_prefix) {
		assert(task_type == JobType::JUICE);
	}

	Task(const Task& task) = delete;
	Task& operator=(const Task& task) = delete;

	Task(Task&& task) : task_id(task.task_id), task_type(task.task_type), executable_file_name(task.executable_file_name), inputs(task.inputs), 
		input_file_prefix(task.input_file_prefix), input_range_start(task.input_range_start), input_range_end(task.input_range_end) {}

	Task& operator=(Task&& task) {
		this->task_id = task.task_id;
		this->task_type = task.task_type;
		this->executable_file_name = task.executable_file_name;
		this->inputs = task.inputs;
		this->input_file_prefix = task.input_file_prefix;
		this->input_range_start = task.input_range_start;
		this->input_range_end = task.input_range_end;

		return *this;
	}

	bool operator==(const Task& task) const {
		return this->task_id == task.task_id &&
				this->task_type == task.task_type &&
				this->executable_file_name == task.executable_file_name &&
				this->inputs == task.inputs &&
				this->input_file_prefix == task.input_file_prefix &&
				this->input_range_start == task.input_range_start &&
				this->input_range_end == task.input_range_end;
	}

	uint32_t get_task_id() const {
		return task_id;
	}

	JobType get_task_type() const {
		return task_type;
	}

	std::string get_executable_file_name() const {
		return executable_file_name;
	}

	std::vector<std::string> get_inputs() const {
		return inputs;
	}

	std::string get_input_file_prefix() const {
		return input_file_prefix;
	}

	std::vector<uint32_t> get_input_range_start() const {
		return input_range_start;
	}

	std::vector<uint32_t> get_input_range_end() const {
		return input_range_end;
	}

	// Returns size of buffer required to serliaze this.
	// The structure of the char buffer for each task type is defined in lucid chart:
	// https://lucid.app/lucidchart/e997188c-bcdf-40c2-84b4-d836f0ceb22e/edit?page=0_0&invitationId=inv_d71841b7-1540-48f6-9b7e-ac7f7bbcfb97#
	uint32_t struct_serialized_size() const {
		uint32_t num_inputs = inputs.size();
		uint32_t input_size = 0;
		for (const std::string& input : inputs) {
			input_size += sizeof(uint32_t) + input.length();
		}
		return sizeof(uint32_t) + sizeof(uint32_t) + // task_id & task_type
			sizeof(uint32_t) + executable_file_name.length() + // length of executable and executable name
			sizeof(uint32_t) + input_file_prefix.length() + // length of input file prefix and input file prefix
			sizeof(uint32_t) + // number of inputs
			input_size +
			(task_type == JobType::MAPLE ? 
				2 * sizeof(uint32_t) * num_inputs : // input start and end range
				0
			);
	}

	// Serializes fields of this and sends it over the socket.
	// The structure of the char buffer for each task type is defined in lucid chart:
	// https://lucid.app/lucidchart/e997188c-bcdf-40c2-84b4-d836f0ceb22e/edit?page=0_0&invitationId=inv_d71841b7-1540-48f6-9b7e-ac7f7bbcfb97#
	uint32_t serialize(int socket) const {
		uint32_t packet_size = struct_serialized_size() + sizeof(uint32_t); // size of footer
		uint32_t network_packet_size = htonl(packet_size);
		send(socket, (char*)&network_packet_size, sizeof(uint32_t), 0);

		uint32_t network_task_id = htonl(static_cast<uint32_t>(task_id));
		send(socket, (char*)&network_task_id, sizeof(uint32_t), 0);

		uint32_t network_task_type = htonl(static_cast<uint32_t>(task_type));
		send(socket, (char*)&network_task_type, sizeof(uint32_t), 0);
	
		uint32_t network_exe_file_name_len = htonl(executable_file_name.length());
		send(socket, (char*)&network_exe_file_name_len, sizeof(uint32_t), 0);
		send(socket, executable_file_name.c_str(), executable_file_name.length(), 0);
	
		uint32_t network_input_prefix_file_name_len = htonl(input_file_prefix.length());
		send(socket, (char*)&network_input_prefix_file_name_len, sizeof(uint32_t), 0);
		send(socket, input_file_prefix.c_str(), input_file_prefix.length(), 0);

		uint32_t network_inputs_len = htonl(inputs.size());
		send(socket, (char*)&network_inputs_len, sizeof(uint32_t), 0);

		for (uint32_t i = 0; i < inputs.size(); ++i) {
			uint32_t network_input_len = htonl(inputs[i].length());
			send(socket, (char*)&network_input_len, sizeof(uint32_t), 0);
			send(socket, inputs[i].c_str(), inputs[i].length(), 0);

			if (task_type == JobType::MAPLE) {
				uint32_t network_input_start = htonl(input_range_start[i]);
				send(socket, (char*)&network_input_start, sizeof(uint32_t), 0);
				
				uint32_t network_input_end = htonl(input_range_end[i]);
				send(socket, (char*)&network_input_end, sizeof(uint32_t), 0);
			}
		}
		
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
			std::cout << print_statement_origin << "Client did not include task in the message" << std::endl;
			return -1;
        	}

		{
			uint32_t network_task_id;
			memcpy((char*)&network_task_id, buffer, sizeof(uint32_t));
			this->task_id = ntohl(network_task_id);
			buffer += sizeof(uint32_t);
		}

		{
			uint32_t network_task_type;
			memcpy((char*)&network_task_type, buffer, sizeof(uint32_t));
			this->task_type = static_cast<JobType>(ntohl(network_task_type));
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
			uint32_t input_prefix_file_name_len;
			memcpy((char*)&input_prefix_file_name_len, buffer, sizeof(uint32_t));
			input_prefix_file_name_len = ntohl(input_prefix_file_name_len);
			buffer += sizeof(uint32_t);

			char* input_prefix_file_name_buffer = new char[input_prefix_file_name_len + 1];
			input_prefix_file_name_buffer[input_prefix_file_name_len] = '\0';
			memcpy(input_prefix_file_name_buffer, buffer, input_prefix_file_name_len);
			this->input_file_prefix = std::string(input_prefix_file_name_buffer);
			delete[] input_prefix_file_name_buffer;
			buffer += input_prefix_file_name_len;
		}

		uint32_t input_len;
		memcpy((char*)&input_len, buffer, sizeof(uint32_t));
		input_len = ntohl(input_len);
		buffer += sizeof(uint32_t);

		for (uint32_t i = 0; i < input_len; ++i) {
			{
				uint32_t input_len;
				memcpy((char*)&input_len, buffer, sizeof(uint32_t));
				input_len = ntohl(input_len);
				buffer += sizeof(uint32_t);

				char* input_buffer = new char[input_len + 1];
				input_buffer[input_len] = '\0';
				memcpy(input_buffer, buffer, input_len);
				this->inputs.emplace_back(input_buffer);
				delete[] input_buffer;
				buffer += input_len;
			}

			if (task_type == JobType::MAPLE) {
				uint32_t network_start_range;
				memcpy((char*)&network_start_range, buffer, sizeof(uint32_t));
				this->input_range_start.push_back(ntohl(network_start_range));
				buffer += sizeof(uint32_t);

				uint32_t network_end_range;
				memcpy((char*)&network_end_range, buffer, sizeof(uint32_t));
				this->input_range_end.push_back(ntohl(network_end_range));
				buffer += sizeof(uint32_t);
			}
		}

		uint32_t network_footer;
		memcpy((char*)&network_footer, buffer, sizeof(uint32_t));
		assert(ntohl(network_footer) == FOOTER);
		buffer += sizeof(uint32_t);

		assert(packet_size == (buffer - buffer_start));
		return packet_size;
	}

	static Task deserialize(int socket, std::string print_statement_origin) {
		Task task;
		task.deserialize_self(socket, print_statement_origin);
		return task;
	}
};
