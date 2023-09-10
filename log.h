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

		//generate a random pattern
		std::string charset = "0123456789abcdefghijklmnopqrstuvwxyz";
		std::random_device rd;
		std::default_random_engine generator(rd());
		std::uniform_int_distribution<int> distribution(0, charset.length() - 1);

		while(count++ < line_number){

			std::string sequence1;
			std::string sequence2;
			std::string sequence3;
			std::string sequence4;
			std::string sequence5;
			std::string sequence6;

			for(int length=0;length<10;length++){
				sequence1+=charset[distribution(generator)];
				sequence2+=charset[distribution(generator)];
				sequence3+=charset[distribution(generator)];
				sequence4+=charset[distribution(generator)];
				sequence5+=charset[distribution(generator)];
				sequence6+=charset[distribution(generator)];
			}
 
			//generate lines that occur in one/some/all logs
			if(machine==1)
				log<<"occur in one log: "<<sequence1<<std::endl;
			if(machine%2==0)
				log<<"occur in half logs: "<<sequence2<<std::endl;
			log<<"occur in all logs: "<<sequence3<<std::endl;

			//generate rare, somewhat frequent and frequent patterns
			int frequency = frequency_distribution(frequency_generator);
			if(frequency<rare)
				log<<"rare pattern: "<<sequence4<<std::endl;
			if(frequency<somewhat_frequent)
				log<<"somewhat frequent pattern: "<<sequence5<<std::endl;
			if(frequency<frequent)
				log<<"frequent pattern: "<<sequence6<<std::endl;
		}
		log.close();
		return;
	}
