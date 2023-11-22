#include <algorithm>
#include <cctype>
#include <iostream>
#include <cassert>
#include <fstream>

std::string get_next_split(std::string& str, std::string delimiter) {
        int i = str.find(delimiter);
	if (i < 0) {
		std::cout << "Could not split " << str << " on " << delimiter << std::endl;
		exit(1);
	}
        std::string substring = str.substr(0, i);
        str.erase(0, i + delimiter.length());
	return substring;
}	

std::string get_file_name(std::string task_id, std::string file, std::string file_start, std::string file_end) {
	return task_id + "_maple_output_" + file + "_" + file_start + "_" + file_end;
}

bool is_number(const std::string &s) {
	return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

int main(int argc, char** argv) {
	if (argc != 4) {
		std::cout << "maple_exe did not get the correct number of inputs" << std::endl;
		return 1;
	}
	std::string task_id = argv[1];
	std::string file_1_info = argv[2];
	std::string file_2_info = argv[3];

	std::string dir_1 = get_next_split(file_1_info, "/");
	std::string file_1 = get_next_split(file_1_info, ":");
	std::ifstream file1_read(dir_1 + "/" + file_1);
	if (!file1_read) {
		assert(false && "file 1 given as input to maple doesn't exist");
	}
	file1_read.close();
	std::string file_1_start = get_next_split(file_1_info, ":");
	std::string file_1_end = file_1_info;
	assert(is_number(file_1_start));
	assert(is_number(file_1_end));

	std::string dir_2 = get_next_split(file_2_info, "/");
	std::string file_2 = get_next_split(file_2_info, ":");
	std::ifstream file2_read(dir_2 + "/" + file_2);
	if (!file2_read) {
		assert(false && "file 2 given as input to maple doesn't exist");
	}
	file2_read.close();
	std::string file_2_start = get_next_split(file_2_info, ":");
	std::string file_2_end = file_2_info;
	assert(is_number(file_2_start));
	assert(is_number(file_2_end));

	std::ofstream file1(dir_1 + "/" + get_file_name(task_id, file_1, file_1_start, file_1_end), std::ios::out);
	if (!file1) {
		std::cout << "could not create file " << get_file_name(task_id, file_1, file_1_start, file_1_end) << std::endl;
		std::perror("file 1 creation failed");
	}
	std::cout << "creating file with name: " << get_file_name(task_id, file_1, file_1_start, file_1_end) << std::endl;
	file1 << file_1_start << std::endl;
	file1 << file_1_end << std::endl;
	file1.close();

	std::ofstream file2(dir_2 + "/" + get_file_name(task_id, file_2, file_2_start, file_2_end), std::ios::out);
	if (!file2) {
		std::cout << "could not create file " << get_file_name(task_id, file_2, file_2_start, file_2_end) << std::endl;
		std::perror("file 2 creation failed");
	}
	std::cout << "creating file with name: " << get_file_name(task_id, file_2, file_2_start, file_2_end) << std::endl;
	file2 << file_2_start << std::endl;
	file2 << file_2_end << std::endl;
	file2.close();

	std::cout << "done..; all files should be created" << std::endl;
	
	return 0;
}
