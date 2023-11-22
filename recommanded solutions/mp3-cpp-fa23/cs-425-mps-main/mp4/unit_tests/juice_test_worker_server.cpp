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

int main(int argc, char** argv) {
	if (argc != 5) {
		std::cout << "juice_exe did not get the correct number of inputs" << std::endl;
		return 0;
	}
	std::string task_id = argv[1];
	std::string file_prefix = argv[2];
	std::string key_1 = argv[3];
	std::string key_2 = argv[3];

	std::ifstream file1(file_prefix + key_1);	
	std::string file_1_start, file_1_end;
	std::getline(file1, file_1_start);
	std::getline(file1, file_1_end);
	get_next_split(key_1, "_"); // skip the file base name
	assert(file_1_start ==  get_next_split(key_1, "_"));
	assert(file_1_end == key_1);
	file1.close();

	std::ifstream file2(file_prefix + key_2);
        std::string file_2_start, file_2_end;
	std::getline(file2, file_2_start);
	std::getline(file2, file_2_end);
        get_next_split(key_2, "_"); // skip the file base name
        assert(file_2_start ==  get_next_split(key_2, "_"));
        assert(file_2_end == key_2);
        file2.close();

	std::ofstream outputfile("./worker_temp_files/" + task_id + "_juice_output", std::ios::out);
	outputfile << file_1_start << std::endl;
	outputfile << file_1_end << std::endl;
	outputfile << file_2_start << std::endl;
	outputfile << file_2_end << std::endl;
	outputfile.close();

	return 0;
}
