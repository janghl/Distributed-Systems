#include <arpa/inet.h>

#include <cassert>
#include <cstring>
#include <chrono>
#include <mutex>
#include <iostream>
#include <fstream>
#include <functional>
#include <netdb.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <unordered_set>
#include <netdb.h>
#include <arpa/inet.h>
#include <fstream>

#include "utils.hpp"
#include "task.hpp"
#include "job.hpp"

#include "../mp1/utils.hpp"
#include "../mp2/utils.hpp"

bool send_task_over_socket(int socket, const Task& task, std::string print_statement_origin) {
	uint32_t payload_size = task.struct_serialized_size();
	uint32_t payload_sent = 0;
	try {
		payload_sent = task.serialize(socket);
	} catch (...) {
		std::cout << print_statement_origin << "Socket closed while trying to serialize task" << std::endl;
		return false;
	}

	if (errno == EPIPE) {
		return false;
	}

	assert(payload_sent == payload_size + sizeof(uint32_t));
	return true;

#ifdef DEBUG_MP4
	std::cout << print_statement_origin << "Sending task over socket" << std::endl;
#endif

	return true;
}

bool get_task_over_socket(int socket, Task& task, std::string print_statement_origin) {
	try {
		int ret = task.deserialize_self(socket, print_statement_origin);
		if (ret < 0) {
			std::cout << print_statement_origin << "Client did not send the correct task header" << std::endl;
			return false;
		}
		if (errno == EPIPE) {
			return false;
		}
	} catch (...) {
		std::cout << print_statement_origin << "Socket closed while trying to deserialize task" << std::endl;
		return false;
	}
	return true;
}

bool send_job_over_socket(int socket, Job& job, std::string print_statement_origin){
	uint32_t payload_size = job.struct_serialized_size();
	uint32_t payload_sent = 0;
	try {
		payload_sent = job.serialize(socket);
	} catch (...) {
		std::cout << print_statement_origin << "Socket closed while trying to serialize task" << std::endl;
		return false;
	}

	if (errno == EPIPE) {
		return false;
	}

	assert(payload_sent == payload_size);
	return true;

#ifdef DEBUG_MP4
	std::cout << print_statement_origin << "Sending task over socket" << std::endl;
#endif

	return true;
}
