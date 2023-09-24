#include "sender.h"

int main(int argc, char** argv){
    if(argc!=2)
        std::cout << "wrong parameter!" << std::endl 
        << "usage: machine number";
    Controller controller;
    controller.controller(argv[1]);
    return 0;
}