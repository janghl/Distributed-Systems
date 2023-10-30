#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>
#include <random>

#include "membership_list_manager.hpp"
#include "gossip_const.h"
#include "gossip_client.h"
#include "mp1_wrapper.hpp"
#include "gossip_const.h"
#include "introducer.hpp"
#include "utils.hpp"

void launch_gossip_client(int gossip_server_port, pthread_t& thread, int introducer_id, int introducer_port, 
		int self_machine_id){
	RunGossipClientArgs* args = new RunGossipClientArgs;
	args->server_port = gossip_server_port;  
	args->introducer_id = introducer_id;  
	args->introducer_port = introducer_port;
    	args->machine_id = self_machine_id;
	pthread_create(&thread, NULL, run_gossip_client, args);

}


void send_leave_to_all(int server_port){
    //set self state as leave 
    membership_list_manager.self_set_state_leave();
    const std::vector<MemberInformation> client_membership_list = membership_list_manager.get_membership_list();
    
    for(const auto& member : client_membership_list) {
        //send message to all
        std::string server_ip = member.get_machine_ip(); 
        uint32_t membership_list_size = client_membership_list.size();
        char* serialized_membership_list = membership_list_manager.serialize_membership_list(membership_list_size, not is_protocol_gossip());
	if (serialized_membership_list == NULL) {
		return;
	}
        int sock_fd = setup_udp_client_socket(server_ip, server_port);
        send_membership_and_protocol(sock_fd, membership_list_manager.get_self_id(), membership_list_manager.get_self_version(),
			serialized_membership_list, membership_list_size);
	delete[] serialized_membership_list;
    }
}

void* run_gossip_client(void* args){
	RunGossipClientArgs* arguments = (RunGossipClientArgs*) args;
	log_thread_started("gossip client");
        while (not end_session) {
            const std::vector<MemberInformation> client_membership_list = membership_list_manager.get_membership_list();

            //pick ramdom b server in alive or suspect and send ping
            std::vector<MemberInformation> live_members;
            for(const auto& member : client_membership_list) {
                if(!member.has_failed()) {
                    live_members.push_back(member);
                }
            }

            std::vector<MemberInformation> b_servers;
            if(live_members.size() > 0) {
                std::random_shuffle(live_members.begin(), live_members.end()); 
                b_servers.assign(live_members.begin(), live_members.begin() + std::min((size_t)GOSSIP_B, live_members.size())); 
            }

            //increment the heartbeat
            membership_list_manager.self_increment_heartbeat_counter();

            //scan the membership list to detect failed/suspected nodes
            unsigned long current = get_current_time();
            unsigned long t_suspected = SUSPECT;
            unsigned long t_fail = FAIL;
            unsigned long t_cleanup = CLEANUP;
	    bool is_gossip_with_suspect = not is_protocol_gossip();
            membership_list_manager.update_membership_list(t_suspected * GOSSIP_CYCLE, t_fail * GOSSIP_CYCLE, t_cleanup * GOSSIP_CYCLE, 
			    is_gossip_with_suspect);

	    // serialize the membership list
            uint32_t membership_list_size = 0;
            char* serialized_membership_list = membership_list_manager.serialize_membership_list(membership_list_size, is_gossip_with_suspect);
	    if (serialized_membership_list == NULL) {
            	std::this_thread::sleep_for(std::chrono::milliseconds(GOSSIP_CYCLE));//500
		continue;
	    }

            // gossip membership list to random servers
            for(const auto& server : b_servers) {
                std::string server_ip = server.get_machine_ip(); 
#ifdef DEBUG
		std::cout << current << " - Gossip to " << server.get_machine_id() << ":" << server.get_machine_version() << std::endl;
		membership_list_manager.print(false);
#endif

                // setup client and send membership & protocol to server
#ifdef TCP
                int sock_fd = setup_tcp_client_socket(server_ip, arguments->server_port);
#else
                int sock_fd = setup_udp_client_socket(server_ip, arguments->server_port); 
#endif
		if (sock_fd < 0) {
			continue;
		}
                // client message has format [generation count - 4 bytes][gossip protocol - 4 bytes][payload size - 4 bytes][payload]
                send_membership_and_protocol(sock_fd, membership_list_manager.get_self_id(), membership_list_manager.get_self_version(), 
				serialized_membership_list, membership_list_size);
		num_gossips += 1;

#ifdef FALSE_POSTIVE_RATE
		if (num_gossips && num_gossips % 300 == 0) {
			std::cout << "False positive rate after " << num_gossips << " gossips: " << failures << " failures " << (static_cast<double>(failures) / num_gossips) << std::endl;
		}
#endif
		close(sock_fd);
            }            
	    delete[] serialized_membership_list;
            std::this_thread::sleep_for(std::chrono::milliseconds(GOSSIP_CYCLE));//500
        }
    delete arguments;
	return NULL;
}
