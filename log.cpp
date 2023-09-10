#include "log.h"

int main(int argc, char** argv){
	if(argc!=2)
		std::cout<<"usage: machine"<<std::endl;
	logging(argv[1], 1000);
	}