#include <iostream>
#include <unistd.h>
#include <fstream>
#include <ios>
#include <cassert>
#include <vector>
#include <string>

int read_file(std::string local_file_name, char*& file_data_ptr, uint32_t& file_size) {
        std::fstream file;
        file.open(local_file_name, std::ofstream::in);
        if (!file) {
                return -1;
        }
        file.seekp(0, std::ios::end);
        file_size = file.tellp();
        file.seekg(0, std::ios::beg);

        file_data_ptr = new char[file_size];
        file.read(file_data_ptr, file_size);

        file.close();
#ifdef DEBUG_MP3_DATA_NODE_CLIENT
        std::cout << "DATA_NODE_CLIENT: " << "reading local file: " << local_file_name << std::endl;
#endif
        return 0;
}


int main() {
	char* data;
	uint32_t data_size;
	read_file("../test_files/gb2.txt", data, data_size);
	std::cout << "Read " << data_size << " bytes into memory at data pointer: " << (void*)data << std::endl;

	uint32_t total_message_size = data_size + 2031;
	char* buffer = new char[total_message_size];
	std::cout << "Allocated memory buffer of " << total_message_size << " bytes into memory at data pointer: " << (void*)buffer << std::endl;
	return 0;
}
