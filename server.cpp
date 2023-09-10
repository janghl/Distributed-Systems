#include "server.h"
#include "log.h"

int main(int argc, char** argv) {
	logging(argv[1], 1000);
	Server server;
	server.Initialize();
	server.Listen();
	server.Connect();
	return 0;
}