#include <initializer_list>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/syscall.h>
#include <random>

inline void logging(std::string command, int line_number){
		std::string pattern1 = "one-log";
		std::string pattern2 = "some-logs";
		std::string pattern3 = "all-logs";
		std::string pattern4 = "rare-pattern";
		std::string pattern5 = "somewhat-frequent-pattern";
		std::string pattern6 = "frequent-pattern";
		int machine = stoi(command);
		std::ofstream log("machine"+std::to_string(machine)+".log");
		if(!log.is_open())
			perror("cannot create file");
		std::cout<<"create file: "<<"machine"+std::to_string(machine)+".log"<<std::endl;
		
		//generate frequency indicator
		double rare = 0.05;
		double somewhat_frequent = 0.5;
		double frequent = 0.8;
		int count = 0;
		std::random_device frequency_rd;
		std::default_random_engine frequency_generator(frequency_rd());
		std::uniform_int_distribution<int> frequency_distribution(0,1);

		while(count++ < line_number){
			//generate lines that occur in one/some/all logs
			if(machine==1)
				log<<"occur in one log: "<<pattern1<<std::endl;
			if(machine%2==0)
				log<<"occur in some logs: "<<pattern2<<std::endl;
			log<<"occur in all logs: "<<pattern3<<std::endl;

			//generate rare, somewhat frequent and frequent patterns
			int frequency = frequency_distribution(frequency_generator);
			if(frequency<rare)
				log<<"rare pattern: "<<pattern4<<std::endl;
			if(frequency<somewhat_frequent)
				log<<"somewhat frequent pattern: "<<pattern5<<std::endl;
			if(frequency<frequent)
				log<<"frequent pattern: "<<pattern6<<std::endl;
		}
		log.close();
		return;
	}
