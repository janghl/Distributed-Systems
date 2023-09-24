#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <mutex>
#include <netdb.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

//

class Controller {
public:
  int controller(int machine) {
    std::thread sender_thread(&Controller::SendHeartbeats, this);
    sender_thread.detach();
    std::thread receiver_thread(&Controller::Receiver, this);
    receiver_thread.detach();
    std::thread checker_thread(&Controller::Checker, this);
    checker_thread.detach();
    std::thread cli_thread(&Controller::Interface, this);
    return 0;
  }

private:
  void Log(const std::string &message) {
    std::ofstream log_file;
    log_mtx_.lock();
    log_file.open("mp2.log", std::ios::app);
    // append to file new line
    log_file << message << std::endl;
    log_file.close();
    log_mtx_.unlock();
    return;
  }

  void Join() {
    auto time = std::chrono::system_clock::now();
    time_stamp_ = time.time_since_epoch().count();
    NodeId node_id{host_, port_, time_stamp_};
    list_mtx_.lock();
    membership_list_.try_emplace(node_id, 0, 0, Status::kAlive);
    list_mtx_.unlock();
    if (is_introducer_) {
      Log("Introducer joined");
    } else {
      std::string message;
      std::stringstream ss(message);
      ss << "JOIN" << std::endl;
      ss << host_ << std::endl;
      ss << port_ << std::endl;
      ss << time_stamp_ << std::endl;
      // Send to introducer host and port
      ssize_t size = sendto(sock_fd_, message.c_str(), message.length(), 0, )
    }

    // cond var to make sure the fd is connected
    // then mutex lock around fd
    // then mutex unlock
    // send message to introducer to help join
  }
  void Leave() {
    list_mtx_.lock();
    auto it = membership_list_.find(NodeId{host_, port_, time_stamp_});
    if (it != membership_list_.end()) {
      Log("Couldn't find self entry in membership list");
      return;
    }
    if (it->second.status != Status::kAlive) {
      Log("Self status isn't alive");
      return;
    }
    // send this to who?
    std::string message;
    std::stringstream ss(message);
    ss << "LEAVE" << std::endl;
    ss << host_ << std::endl;
    ss << port_ << std::endl;
    ss << time_stamp_ << std::endl;
    // figure this out later
    ssize_t size = sendto(sock_fd_, message.c_str(), message.length(), 0, )
    // send gossip to a few about leaving
    list_mtx_.unlock();
  }
  void List() {
    std::cout << "Membership List on host " + host_ << std::endl;
    list_mtx_.lock();
    for (const auto &[key, entry] : membership_list_) {
      std::string status_string = status_to_string_[entry.status];
      printf("Host: %s, count: %d, local_time: %d, status: %s",
             key.host.c_str(), entry.count, entry.local_time,
             status_string.c_str());
      std::cout << std::endl;
    }
    list_mtx_.unlock();
  }
  void Monitor() {
    std::cout << "Commands: join, leave, list" << std::endl;
    std::cout << "Input: ";
    std::string command;
    while (std::cin >> command) {
      if (command == "join") {
        Join();
      } else if (command == "leave") {
        Leave();
      } else if (command == "list") {
        List();
      } else {
        std::cout << "Unidentified command, use one of join, leave, list"
                  << std::endl;
      }
      std::cout << "Input: ";
    }
  }
  std::string ListToString() const {
    std::string retval;
    std::stringstream ss(retval);
    for (const auto &[key, value] : membership_list_) {
      ss << key.host << "," << key.port << "," << key.time_stamp << ","
         << value.count << "," << value.local_time << ","
         << status_to_string_.at(value.status) << " ";
    }
    ss << std::endl;
    return retval;
  }
  enum class Status {
    kAlive,
    kFailed,
    kSuspected,
  };
  std::unordered_map<std::string, Status> string_to_status_ = {
      {"alive", Status::kAlive},
      {"suspected", Status::kSuspected},
      {"failed", Status::kFailed}};
  std::unordered_map<Status, std::string> status_to_string_ = {
      {Status::kAlive, "alive"},
      {Status::kSuspected, "suspected"},
      {Status::kFailed, "failed"}};
  enum class Command {
    kJoin,
    kLeave,
    kList,
  };
  struct NodeId {
    std::string host;
    std::string port;
    long time_stamp;
  };
  struct MembershipEntry {
    int count;
    int local_time;
    Status status;
  };
  std::map<NodeId, MembershipEntry> StringToList(const std::string &str) const {
    std::map<NodeId, MembershipEntry> map;
    std::string entry;
    while (std::stringstream(str) >> entry) {
      std::vector<std::string> splitted;
      std::string substr;
      while (getline(std::stringstream(entry), substr, ',')) {
        splitted.push_back(substr);
      }
      map.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(splitted[0], splitted[1],
                                std::stol(splitted[2])),
          std::forward_as_tuple(std::stoi(splitted[3]), std::stoi(splitted[4])),
          string_to_status_.at(splitted[5]));
    }
  }
  const int kTFail = 30;
  const int kTCleanup = 20;
  const int kTSuspect = 20;
  const int kHeartbeatInterval = 20;
  const int kScale = 10;
  std::mutex list_mtx_;
  std::mutex log_mtx_;
  std::map<NodeId, MembershipEntry> membership_list_;
  bool using_suspicion_;
  int machine_;
  std::string host_;
  std::string port_;
  long time_stamp_;
  int sock_fd_;
  bool is_introducer_;
  int Receiver(int machine, struct MembershipEntry *list) {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct addrinfo hints, *infoptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    int s = getaddrinfo(NULL, "8080", &hints, &infoptr);
    if (s) {
      perror("receiver cannot set address!");
      return 1;
    }
    if (bind(sock_fd, infoptr->ai_addr, infoptr->ai_addrlen) == -1) {
      perror("receiver cannot bind!");
      return 1;
    }
    std::cout << "bound!";
    struct sockaddr_storage addr;
    int addrlen = sizeof(addr);
    while (true) {
      auto iter = membership_list_.find(NodeId{host_, port_, time_stamp_});
      if (iter == membership_list_.end()) {
        continue;
      }
      char buffer[4096];
      // get other fd
      int other_fd;
      int client_len;
      struct sockaddr_in client_addr;
      ssize_t size =
          recvfrom(sock_fd, buffer, 4096, 0, (struct sockaddr *)&client_addr,
                   (socklen_t *)&client_len);
      std::string result(buffer);
      std::stringstream ss(result);
      std::string line;
      ss >> line;
      if (line == "DATA") {
        std::string rest;
        std::getline(ss, rest);
        std::map<NodeId, MembershipEntry> other_list = StringToList(rest);

      } else {
        std::string host;
        std::string port;
        long time_stamp;
        ss >> host >> port >> time_stamp;
        ss >> port;
        ss >> time_stamp;
        if (line == "LEAVE") {
          list_mtx_.lock();
          membership_list_.erase(iter);
          list_mtx_.unlock();
          Log("Leave detected on local membership list");
        } else if (line == "JOIN") {
          // add to membership list
          // LOG
          // send my membership list to new machine
          NodeId node{host, port, time_stamp};
          MembershipEntry entry{0, 0, Status::kAlive};
          list_mtx_.lock();
          membership_list_[node] = entry;
          list_mtx_.unlock();
          std::string data;
          std::stringstream oss(data);
          oss << "DATA" << std::endl;
          oss << ListToString();
        }
      }
      if (line == "LEAVE") {
        list_mtx_.lock();
      }
      std::map<NodeId, MembershipEntry> other_list =
          StringToList(std::string(buffer));

      char *buffer = new char[kScale * sizeof(struct MembershipEntry)];
      struct MembershipEntry *recvlist = new struct MembershipEntry[Scale];
      int byte_count =
          recvfrom(sock_fd, buffer, Scale * sizeof(struct MembershipEntry), 0,
                   &addr, &addrlen); // recv
      if (byte_count > 0)
        std::cout << "received " << byte_count << " bytes successfully!"
                  << std::endl;
      else
        std::cout << "receive failed!" << std::endl;
      std::memcpy(recvlist, buffer, Scale * sizeof(struct MembershipEntry));
      merge(list, recvlist, machine);
    }
    return 0;
  }
  // membership_list_: list1, other: list2
  int merge(const std::map<NodeId, MembershipEntry> &other,
            int machine) {
    std::lock_guard<std::mutex> lock(mutex);
    for (int num = 0; num < Scale && num != machine; num++) {
      if (list1[num].node_id == "\0" && list2[num].node_id != "\0") {
        list1[num].status = list2[num].status;
        list1[num].count = list2[num].count;
        list1[num].local_time = list1[machine].local_time;
        std::cout << "create new entry " << num << std::endl;
      }
      if (suspicion == false) {
        if (list2[num].status != alive && list1[num].status == alive) {
          list1[num].status = list2[num].status;
          std::cout << "machine " << num << " status changed to "
                    << list1[num].status << std::endl;
        } else if (list2[num].count > list1[num].count) {
          list1[num].count = list2[num].count;
          list1[num].local_time = list2[machine].local_time;
          std::cout << "updated entry " << num << " on machine " << machine
                    << std::endl;
        } else {
          if (list2[num].status == failed || list1[num].status == failed) {
            list1[num].status = list2[num].status = failed;
            std::cout << "machine " << num << " has failed!" << std::endl;
          } else {
            if (list2[num].count > list1[num].count) {
              list1[num].count = list2[num].count;
              list1[num].local_time = list2[machine].local_time;
              std::cout << "updated entry " << num << " on machine " << machine
                        << std::endl;
            }
            if (list2[num].status != list1[num].status) {
              list1[num].status = list2[num].status;
              std::cout << "machine " << num << " status changed to "
                        << list1[num].status << std::endl;
            }
          }
        }
      }
    }
    return 0;
  }
};

void delete() {
  for (const auto &[key, value] : list1) {
  key:
    NodeId;
  value:
    MembershipEntry;
    auto list2iterator = list2.find(key);
    list2iterator->first : NodeId;
    list2iterator->second : MembershipEntry;
  }
}

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

int sender(int id, struct MembershipList *list) {
  sock_fd_ = socket(AF_INET, SOCK_DREAM, IPPROTO_UDP);
  struct addrinfo hints, *infoptr;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DREAM;
  int addr = getaddrinfo(hosts[id], "8080", &hints, &infoptr);
  if (addr) {
    perror("sender cannot set address!");
    return 1;
  }
  if (connect(sock_fd, infoptr->ai_addr, infoptr->ai_addrlen) == -1) {
    perror("sender cannot connect! target machine number = " + id);
    return 1;
  }
  char *buffer = new char[Scale * sizeof(struct MembershipList)];
  std::memcpy(buffer, list, Scale * sizeof(struct MembershipList));
  if (send(sock_fd, buffer, Scale * sizeof(struct MembershipList), 0) == -1) {
    perror("sender cannot send to machine " + id);
    return 1;
  }
  close(sock_fd);
  return 0;
  int sender(int id, struct MembershipEntry *list) {
    int sock_fd = socket(AF_INET, SOCK_DREAM, IPPROTO_UDP);
    struct addrinfo hints, *infoptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DREAM;
    int addr = getaddrinfo(hosts[id], "8080", &hints, &infoptr);
    if (addr) {
      perror("sender cannot set address!");
      return 1;
    }
    if (connect(sock_fd, infoptr->ai_addr, infoptr->ai_addrlen) == -1) {
      perror("sender cannot connect! target machine number = " + id);
      return 1;
    }
    char *buffer = new char[Scale * sizeof(struct MembershipEntry)];
    std::memcpy(buffer, list, Scale * sizeof(struct MembershipEntry));
    if (send(sock_fd, buffer, Scale * sizeof(struct MembershipEntry), 0) ==
        -1) {
      perror("sender cannot send to machine " + id);
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
        list_mtx_.lock();
        list[machine].count++;
        list[machine].local_time++;
        srand (time(NULL));
        int target;
        while(machine == (target = rand()%10+1));
        sender(target, list);
        list_mtx_.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_interval));
  /*  background thread
      generate and update my own heartbeats and local time
      send heartbeats to a random neighbour each 20ms
  */
  // change logic so that
  // 2 3 4 5 6 7 8 9 10
  // 2 5 8 7 6 4 3 10 9
  // 2n - 1
  //
  int send_heartbeats(int machine, struct MembershipEntry *list) {
    while (true) {
      std::lock_guard<std::mutex> lock(mutex);
      list[machine].count++;
      list[machine].local_time++;
      srand(time(NULL));
      int target;
      while (machine == (target = rand() % 10 + 1))
        ;
      sender(target, list);
      std::this_thread::sleep_for(
          std::chrono::milliseconds(heartbeat_interval));
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

  // unit test this too
  int checker(int machine, struct MembershipEntry *list) {
    for (int num = 0; num < Scale && num != machine; num++) {
      if (list[num].local_time - list[machine].local_time >= T_fail) {
        if (suspicion == true) {
          list[num].status = suspected;
          if (list[num].local_time - list[machine].local_time >=
              T_fail + T_suspect)
            list[num].status = failed;
          if (list[num].local_time - list[machine].local_time >=
              T_fail + T_suspect + T_cleanup)
            erase[num];
// write unit tests for this
// 
// TODO: specify merge in place, use const static
/*  create neighbour's entry;
    detect status change;
    merging received list to local one;
    usage: local membership list, received membership list, machine number
*/
int merge(struct MembershipList *list1, struct MembershipList *list2,
          int machine) {
  for (int num = 0; num < Scale && num != machine; num++) {
    if (list1[num].node_id == "\0" && list2[num].node_id != "\0") {
      list1[num].status = list2[num].status;
      list1[num].count = list2[num].count;
      list1[num].local_time = list1[machine].local_time;
      std::cout << "create new entry " << num << std::endl;
    }
    if (suspicion == false) {
      if (list2[num].status != alive && list1[num].status == alive) {
        list1[num].status = list2[num].status;
        std::cout << "machine " << num << " status changed to "
                  << list1[num].status << std::endl;
      } else if (list2[num].count > list1[num].count) {
        list1[num].count = list2[num].count;
        list1[num].local_time = list2[machine].local_time;
        std::cout << "updated entry " << num << " on machine " << machine
                  << std::endl;
      } else {
        if (list2[num].status == failed || list1[num].status == failed) {
          list1[num].status = list2[num].status = failed;
          std::cout << "machine " << num << " has failed!" << std::endl;
        } else {
          list[num].status = failed;
          if (list[num].local_time - list[machine].local_time >=
              T_fail + T_cleanup)
            vector[num].push;
        }
      }
          if (list2[num].count > list1[num].count) {
            list1[num].count = list2[num].count;
            list1[num].local_time = list2[machine].local_time;
            std::cout << "updated entry " << num << " on machine " << machine
                      << std::endl;
          }
          if (list2[num].status != list1[num].status) {
            list1[num].status = list2[num].status;
            std::cout << "machine " << num << " status changed to "
                      << list1[num].status << std::endl;
          }
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
        list_mtx_.lock();
        merge(list, recvlist, machine);
        list_mtx_.unlock();
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
    if (list[machine].status = suspected)
      list[machine].status = alive;
  }

  int controller(int machine) {
    struct MembershipEntry[Scale] list;

    // initialize my own node_id
    std::string timestamp = std::to_string(((int)time(NULL)) % 60);
    list[machine].node_id = str(hosts[machine] + " 8080 " + timestamp);

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