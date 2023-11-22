#include <string>

#include "operation_type.hpp"

OperationType is_sdfs_command(std::string command) {
        if (command.length() >= 2 && command.substr(0, 2) == "ls") {
                return OperationType::LS;
        } else if (command.length() >= 3 && command.substr(0, 3) == "put") {
                return OperationType::WRITE;
        } else if (command.length() >= 3 && command.substr(0, 3) == "get") {
                return OperationType::READ;
        } else if (command.length() >= 6 && command.substr(0, 6) == "delete") {
                return OperationType::DELETE;
        }
        return OperationType::INVALID;
}

std::string operation_to_string(OperationType operation) {
	switch(operation) {
		case OperationType::WRITE:
			return "Write";
		case OperationType::READ:
			return "Read";
		case OperationType::DELETE:
			return "Delete";
		case OperationType::LS:
			return "LS";
		default:
			return "Invalid";
	}
}
