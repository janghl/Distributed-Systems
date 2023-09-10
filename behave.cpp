#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include "controller.h"
int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: behave pattern\n");
  }
  std::vector<FILE *> vms;
  int machines = 1;
  vms.reserve(machines);
  for (int i = 1; i <= machines; ++i) {
    char buffer[64];
    sprintf(buffer, "ssh js66@fa23-cs425-69%02d.cs.illinois.edu", i);
    // ssh into other box
    FILE *vm = popen(buffer, "w");
    std::cout << "Opened ssh" << std::endl;

    // start server as background process
    fprintf(vm, "cd /home/js66/distributed-log-querier\n");
    std::cout << "Called cd" << std::endl;
    fprintf(vm, "make server\n");
    std::cout << "Called make" << std::endl;
    fprintf(vm, "./server.out %d &\n", i);
    std::cout << "Run executable" << std::endl;
    vms.push_back(vm);
  }
  sleep(10);
  std::cout << "Started up all server" << std::endl;
  Controller controller(argv[1], machines);
  controller.DistributedGrep();
  for (FILE *vm : vms) {
    fprintf(vm, "pkill server\n");
    pclose(vm);
  }
  return 0;
}