#include <string>

#include "utils.hpp"
#include "../mp1/utils.hpp"
#include "membership_list_manager.hpp"
extern std::vector<std::string> machine_ips;

struct RunGossipClientArgs {
    int server_port;
    int introducer_id;
    int introducer_port;
    int machine_id;
};

void launch_gossip_client(int gossip_server_port, pthread_t& thread, int introducer_id, int introducer_port, int self_machine_id);

void send_leave_to_all(int server_port);

void* run_gossip_client(void* args);



