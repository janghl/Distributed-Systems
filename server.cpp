#include "server.h"


int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: server vm_number\n");
	  return 1;
  }
  Server server;
  server.StartServer();
  // unreachable
  return 0;
}