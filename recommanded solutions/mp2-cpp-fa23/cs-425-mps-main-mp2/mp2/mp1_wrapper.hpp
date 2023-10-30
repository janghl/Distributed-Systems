#include <arpa/inet.h>

#include <cstring>
#include <iostream>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "../mp1/utils.hpp"
#include "../mp1/client.hpp"
#include "../mp1/server.hpp"

extern std::string machine_id;
void launch_grep_server(pthread_t* grep_server_thread);
