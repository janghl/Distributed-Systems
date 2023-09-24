#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <netdb.h>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include "unit_test.h"
#define T_fail 30
#define T_cleanup 20
#define T_suspect 20
#define heartbeat_interval 20   //heart rate
#define Scale 10                //scale of machines

enum class Status {alive, failed, suspected}; // enum class
std::mutex mutex; // shouldn't be in global scope and rename to be specific
bool suspicion = false; // shouldn't be in global scope
// 
public struct MembershipEntry{ // named wrong MembershipEntry
    std::string node_id = "\0";        //ip+port+timestamp, membership id can be struct
    int count = 0;
    int local_time = 0;
    enum Status status = alive;
};
const static char *hosts[] = {
    "fa23-cs425-6901.cs.illinois.edu", "fa23-cs425-6902.cs.illinois.edu",
    "fa23-cs425-6903.cs.illinois.edu", "fa23-cs425-6904.cs.illinois.edu",
    "fa23-cs425-6905.cs.illinois.edu", "fa23-cs425-6906.cs.illinois.edu",
    "fa23-cs425-6907.cs.illinois.edu", "fa23-cs425-6908.cs.illinois.edu",
    "fa23-cs425-6909.cs.illinois.edu", "fa23-cs425-6910.cs.illinois.edu",
};

/*  sending heartbeats(membership list) to hosts[id]
    return 0 if succeeded, return 1 if failed
    usage: target machine number, membership list
*/

int sender(int id, struct MembershipEntry* list){
    int sock_fd = socket(AF_INET, SOCK_DREAM, IPPROTO_UDP);
    struct addrinfo hints, *infoptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DREAM;
    int addr = getaddrinfo(hosts[id], "8080", &hints, &infoptr);
    if(addr){
          perror("sender cannot set address!");
          return 1;
    }
    if(connect(sock_fd, infoptr->ai_addr, infoptr->ai_addrlen) == -1){
        perror("sender cannot connect! target machine number = "+id);
        return 1;
    }
    char *buffer = new char[Scale * sizeof(struct MembershipEntry)];
    std::memcpy(buffer, list, Scale * sizeof(struct MembershipEntry));
    if(send(sock_fd, buffer, Scale * sizeof(struct MembershipEntry), 0) == -1){
        perror("sender cannot send to machine "+id);
        return 1;
    }
    close(sock_fd);
    return 0;
}

/*  background thread
    generate and update my own heartbeats and local time
    send heartbeats to a random neighbour each 20ms
*/
// change logic so that 
// 2 3 4 5 6 7 8 9 10
// 2 5 8 7 6 4 3 10 9
// 2n - 1
// 
int send_heartbeats(int machine, struct MembershipEntry* list){
    while(true){
        std::lock_guard<std::mutex> lock(mutex);
        list[machine].count++;
        list[machine].local_time++;
        srand (time(NULL));
        int target;
        while(machine == (target = rand()%10+1));
        sender(target, list);
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_interval));
    }
    return 0;
}


// write unit tests for this
// 
// TODO: specify merge in place, use const static
/*  create neighbour's entry;
    detect status change; 
    merging received list to local one;
    usage: local membership list, received membership list, machine number
*/
int merge(struct MembershipEntry* list1, struct MembershipEntry* list2, int machine){
    std::lock_guard<std::mutex> lock(mutex);
    for(int num=0; num<Scale && num!= machine; num++){
        if(list1[num].node_id=="\0" && list2[num].node_id!="\0"){
           list1[num].status = list2[num].status;
            list1[num].count = list2[num].count;
            list1[num].local_time = list1[machine].local_time;
            std::cout<< "create new entry "<<num<<std::endl;
        }
        else if(suspicion == false){
            if(list2[num].status!=alive && list1[num].status==alive){
        list1[num].status = list2[num].status;
        std::cout<< "machine "<<num<<" status changed to "<<list1[num].status<<std::endl;
        }
            else if(list2[num].count > list1[num].count){
        list1[num].count = list2[num].count;
        list1[num].local_time = list2[machine].local_time;
        std::cout<< "updated entry "<<num<<" on machine "<<machine<<std::endl;
        }
        }
        else
        {
            if(list2[num].status==failed || list1[num].status==failed){
                list1[num].status = list2[num].status = failed;
                std::cout<< "machine "<<num<<" has failed!"<<std::endl;
            }
            else
            {
                if(list2[num].count > list1[num].count){
                    list1[num].count = list2[num].count;
                    list1[num].local_time = list2[machine].local_time;
                    std::cout<< "updated entry "<<num<<" on machine "<<machine<<std::endl;
                    }
                if(list2[num].status!=list1[num].status){       
                    list1[num].status = list2[num].status;
                    std::cout<< "machine "<<num<<" status changed to "<<list1[num].status<<std::endl;
                }

            }

        
        }
        
    }
    return 0;
}


/*  background thread
    open service to others
    modify my own list when receiving a package
    return 0 if succeeded, return 1 if failed
*/
int receiver(int machine, struct MembershipEntry* list){
    int sock_fd = socket(AF_INET, SOCK_DREAM, IPPROTO_UDP);
    struct addrinfo hints, *infoptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DREAM;
    hints.ai_flags = AI_PASSIVE;
    int s = getaddrinfo(NULL, "8080", &hints, &infoptr);
    if(s){
        perror("receiver cannot set address!");
        return 1;
    }
    if(bind(sock_fd, infoptr.ai_addr, infoptr.ai_addrlen) == -1){
        perror("receiver cannot bind!");
        return 1;
    }
    std::cout<<"bound!";
    struct sockaddr_storage addr;
    int addrlen = sizeof(addr);
    while(true){
        char *buffer = new char[Scale * sizeof(struct MembershipEntry)];
        struct MembershipEntry* recvlist = new struct MembershipEntry[Scale];
        int byte_count = recvfrom(sock_fd, buffer, Scale * sizeof(struct MembershipEntry),0, &addr, &addrlen); // recv
        if(byte_count > 0)
            std::cout<<"received "<< byte_count<< " bytes successfully!"<<std::endl;
        else
            std::cout<<"receive failed!"<<std::endl;
        std::memcpy(recvlist, buffer, Scale * sizeof(struct MembershipEntry));
        merge(list, recvlist, machine);
    }
    return 0;
}

// unit test this too
int checker(int machine, struct MembershipEntry* list){
    for(int num=0; num<Scale && num!= machine; num++){
        if(list[num].local_time-list[machine].local_time>=T_fail){
            if(suspicion==true){
                list[num].status = suspected;
                if(list[num].local_time-list[machine].local_time>=T_fail+T_suspect)
                    list[num].status = failed;
                if(list[num].local_time-list[machine].local_time>=T_fail+T_suspect+T_cleanup)
                    erase[num];
            }
            else {
                list[num].status = failed;
                if(list[num].local_time-list[machine].local_time>=T_fail+T_cleanup)
                    vector[num].push;
            }
        }
    }
    if(list[machine].status = suspected)
        list[machine].status = alive;
}

int controller(int machine){
    struct MembershipEntry[Scale] list;

    //initialize my own node_id
    std::string timestamp = std::to_string(((int)time(NULL))%60);
    list[machine].node_id = str(hosts[machine]+" 8080 "+timestamp);

    // multithread sender
    std::thread sender_thread{&send_heartbeats, this, machine, list};
    sender_thread.detach();

    // multithread receiver
    std::thread receiver_thread{&receiver, this, machine, list};
    receiver_thread.detach();
    
    // multithread checker
    std::thread checker_thread{&checker, this, machine, list};
    checker_thread.detach();

    return 0;


}