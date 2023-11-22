#pragma once

#include "../../mp2/membership_list_manager.hpp"

// ==== START OF EXTERN VARIABLES
std::string machine_id;
std::vector<std::string> machine_ips;
int server_port;
int grep_server_port = 4096;            // port on which server listens for incoming grep requests
pthread_t grep_server_thread;           // reference to thread running server on port grep_server_port

int introducer_server_port = 1024;      // port on which server listens for messages from new nodes
pthread_t introducer_server_thread;     // reference to thread running server on port introducer_server_port

int gossip_server_port = 2048;          // port on which server listens for heartbeats/membership lists from other nodes
pthread_t gossip_server_thread;         // reference to thread running server on port gossip_server_thread
pthread_t gossip_client_thread;         // reference to thread running client logic for gossip style failure detection

bool is_introducer_running;             // set to true if the current process is also running the introducer

bool end_session = false;               // set to true to get all threads to exit
bool exit_program = false;              // set to true to exit program

int message_drop_rate = 0;              // induced packet drop rate
MembershipListManager membership_list_manager;          // manages the membership list
BlockReport* block_report = nullptr;

std::mutex protocol_mutex;                              // mutex taken by client/server whenever they access/update the generation count
// and membership_protocol
int generation_count;                                   // generation count of membership protocol. Every time a node
// changes the protocol, it updates the generation count and begins
// sending gossip messages with the new generation count and gossip protocol
// piggybacked on it
enum membership_protocol_options membership_protocol;   // reference to the type of failure detection protocol to run (can change
// mid execution)

// Variables to measure bandwdith
std::chrono::time_point<std::chrono::steady_clock> system_join_time;    // record time when machine joined system
uint64_t bytes_sent;                                    // total bytes the client gossiped to others
uint64_t bytes_received;                                // total bytes the server received

// Varibles to measure failures
uint64_t failures = 0;
uint64_t num_gossips = 0;
// ==== END OF EXTERN VARIABLES
