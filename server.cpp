#include "server.h"
#include "log.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: server vm_number\n");
	  return 1;
  }
  logging(argv[1], 1000);
  Server server;
  server.StartServer();
  // unreachable
  return 0;
}