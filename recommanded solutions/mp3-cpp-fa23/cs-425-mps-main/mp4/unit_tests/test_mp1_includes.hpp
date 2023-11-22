#pragma once

// ==== START OF EXTERN VARIABLES
std::string machine_id;
std::vector<std::string> machine_ips;
int grep_server_port = 4096;            // port on which server listens for incoming grep requests
pthread_t grep_server_thread;           // reference to thread running server on port grep_server_port

bool end_session = false;               // set to true to get all threads to exit
bool exit_program = false;              // set to true to exit program
int server_port;
// ==== END OF EXTERN VARIABLES
