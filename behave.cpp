#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include "controller.h"
int main(int argc, char *argv[]) {
  std::vector<FILE *> vms;
  int machines = 2;
  vms.reserve(machines);
  for (int i = 1; i <= machines; ++i) {
    char buffer[64];
    sprintf(buffer, "ssh js66@fa23-cs425-69%02d.cs.illinois.edu", i);
    // ssh into other box
    FILE *vm = popen(buffer, "w");

    // start server as background process
    fprintf(vm, "cd /home/js66/distributed-log-querier\n");
    sleep(1);
    fprintf(vm, "make server\n", i);
    sleep(1);
    fprintf(vm, "./server.out %d &\n", i);
    vms.push_back(vm);
    sleep(2);
  }
  std::cout << "Started up all server" << std::endl;
  char pattern[] = "ab1";
  Controller controller(pattern, machines);
  controller.DistributedGrep();
  for (FILE *vm : vms) {
    fprintf(vm, "pkill server");
    pclose(vm);
  }
  return 0;
}