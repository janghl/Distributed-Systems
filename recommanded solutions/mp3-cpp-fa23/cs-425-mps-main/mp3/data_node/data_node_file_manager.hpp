#pragma once

#include <string>

#include "data_node_block_report_manager.hpp"

class Request; // forward declaration

int file_read(const std::string& file_name, char*& file_data, uint32_t& file_data_length, uint32_t& file_counter, BlockReport* block_report_manager);
int file_write(const std::string& file_name, Request& request, uint32_t file_data_length, uint32_t new_file_update_counter, 
		BlockReport* block_report_manager);
int file_delete(const std::string& file_name, BlockReport* block_report_manager);
