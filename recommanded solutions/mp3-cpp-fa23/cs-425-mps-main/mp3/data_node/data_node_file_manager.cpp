#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <ios>
#include <string>

#include "data_node_block_report_manager.hpp"
#include "data_node_file_manager.hpp"
#include "request.hpp"

#define GET_FILE_PATH(block_report_manager, file_name) (block_report_manager->get_file_system_root() + "/" + file_name)

int file_read(const std::string& file_name, char*& file_data, uint32_t& file_data_size, uint32_t& file_counter, BlockReport* block_report_manager) {
	block_report_manager->lock();
	file_counter = block_report_manager->get_file_update_counter(file_name);
#ifdef DEBUG_MP3_DATA_NODE_FILE_MANAGER
	std::cout << "DATA_NODE_FILE_MANAGER: file " << file_name << " reading file " << file_name << " with update counter " << file_counter << std::endl;
#endif
	block_report_manager->unlock();
	if (file_counter < 0) { // file doesn't exist
		return 0;
	}

	std::fstream file;
	file.open(GET_FILE_PATH(block_report_manager, file_name), std::ofstream::in);
	if (!file) {
		block_report_manager->unlock();
		return -1;
	}

	file.seekp(0, std::ios::end);
	file_data_size = file.tellp();
	file.seekg(0, std::ios::beg);

	file_data = new char[file_data_size];
	file.read(file_data, file_data_size);

	file.close();
	return file_data_size;
}       

int file_write(const std::string& file_name, Request& request, uint32_t file_data_length, uint32_t new_file_update_counter, 
		BlockReport* block_report_manager) {
	std::string temp_file_name = GET_FILE_PATH(block_report_manager, file_name) + ":" + std::to_string(new_file_update_counter);
	int file = open(temp_file_name.c_str(), O_CREAT | O_DSYNC | O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXO);
#ifdef DEBUG_MP3_DATA_NODE_FILE_MANAGER
	std::cout << "DATA_NODE_FILE_MANAGER: file " << file_name << " full path of file: " << (block_report_manager->get_file_system_root() + "/" + file_name) << std::endl;
#endif
	if (file < 0) {
		std::cout << "DATA_NODE_FILE_MANAGER: file " << file_name << " could not open file for writing" << std::endl;
		std::perror("ofsteam open failed");
		return -1;
	}

	char* buffer = nullptr;
	int32_t buffer_size = 0;
	uint32_t total_data_wrote = 0;
	while ((buffer_size = request.read_data_block(buffer, "DATA_NODE_FILE_MANAGER: ")) > 0) {
		write(file, buffer, buffer_size);

#ifdef DEBUG_MP3_DATA_NODE_FILE_MANAGER
                std::cout << "DATA_NODE_FILE_MANAGER: file " << file_name << " wrote " << total_data_wrote << " bytes out of " << request.get_data_length() << std::endl;
				// std::cout << "DATA_NODE_FILE_MANAGER: " << "writing data in hex `" << std::hex << uint32_t(buffer[0]) << uint32_t(buffer[1]) << uint32_t(buffer[2]) 
				// 			<< uint32_t(buffer[3]) << std::dec << "`. total data size: " << request.get_data_length() << std::endl;
#endif
		total_data_wrote += buffer_size;
		delete[] buffer;
		buffer = nullptr;
		buffer_size = 0;
	}

	close(file);

	block_report_manager->lock();
	uint32_t current_counter = block_report_manager->get_file_update_counter(file_name);
	if (current_counter >= new_file_update_counter) {
		block_report_manager->unlock();
		unlink(temp_file_name.c_str());
		std::cout << "DATA_NODE_FILE_MANAGER: file " << file_name << " current file counter is greater than the new counter requested by write; ignoring write" << std::endl;
		return 0;
	} else if (buffer_size == 0) {
		block_report_manager->set_file_update_counter(file_name, new_file_update_counter);
		block_report_manager->unlock();
		std::rename(temp_file_name.c_str(), GET_FILE_PATH(block_report_manager, file_name).c_str());
#ifdef DEBUG_MP3_DATA_NODE_FILE_MANAGER
		std::cout << "DATA_NODE_FILE_MANAGER: file " << file_name << " wrote file and set update counter to " << block_report_manager->get_file_update_counter(file_name) << std::endl;
#endif
		return file_data_length;
	} else {
#ifdef DEBUG_MP3_DATA_NODE_FILE_MANAGER
		std::cout << "DATA_NODE_FILE_MANAGER: file " << file_name << " write request failed because name node closed socket before sending all the data " << std::endl;
#endif
		block_report_manager->unlock();
		unlink(temp_file_name.c_str());
	}

	return -1;
}

int file_delete(const std::string& file_name, BlockReport* block_report_manager) {
	block_report_manager->lock();

	block_report_manager->delete_file(file_name);
	std::remove(GET_FILE_PATH(block_report_manager, file_name).c_str());

	block_report_manager->unlock();
	return 0;
}
