#pragma once

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
#include <fstream>

//

class Controller {
public:
  int controller(int machine) {
    std::thread sender_thread(&Controller::Client, this);
    sender_thread.detach();
    std::thread receiver_thread(&Controller::Receiver, this);
    receiver_thread.detach();
    std::thread checker_thread(&Controller::Checker, this);
    checker_thread.detach();
    std::thread cli_thread(&Controller::Monitor, this);
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
    self_node_ = NodeId{host_, port_, time_stamp_};
    list_mtx_.lock();
    membership_list_.emplace(std::piecewise_construct, std::forward_as_tuple(host_, port_, time_stamp_), std::forward_as_tuple(0, 0, Status::kAlive));
    list_mtx_.unlock();
    if (is_introducer_) {
      Log("Introducer joined");
    } else {
      Log("Non-introducer joined");
      std::string message;
      std::stringstream ss(message);
      ss << "JOIN" << std::endl;
      ss << host_ << std::endl;
      ss << port_ << std::endl;
      ss << time_stamp_ << std::endl;
      // Send to introducer host and port
      ssize_t size = sendto(sock_fd_, message.c_str(), message.length(), 0,
                            &introducer_addr_, addrlen_);
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
      Log("Only alive node can voluntarily leave");
      return;
    }
    it->second.status = Status::kLeft;
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
      } else if (command == "switch") {
        if (using_suspicion_) {
          Log("Switch off suspicion");
          using_suspicion_ = false;
        } else {
          Log("Switch on suspicion");
          using_suspicion_ = true;
        }
      } else {
        std::cout
            << "Unidentified command, use one of join, leave, list, or switch"
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
    kLeft
  };
  std::unordered_map<std::string, Status> string_to_status_ = {
      {"alive", Status::kAlive},
      {"suspected", Status::kSuspected},
      {"failed", Status::kFailed},
      {"left", Status::kLeft}};
  std::unordered_map<Status, std::string> status_to_string_ = {
      {Status::kAlive, "alive"},
      {Status::kSuspected, "suspected"},
      {Status::kFailed, "failed"},
      {Status::kLeft, "left"}};
  enum class Command {
    kJoin,
    kLeave,
    kList,
  };
  struct NodeId {
    std::string host;
    std::string port;
    long time_stamp;
    bool operator==(const NodeId &other) const {
      return (host == other.host) && (port == other.port) && (time_stamp == other.time_stamp);
    }
  };
  struct MembershipEntry {
    int count;
    int local_time;
    Status status;
    bool operator==(const MembershipEntry &other) const {
      return (count == other.count) && (local_time == local_time) && (status == other.status);
    }
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
  const int kTargets = 2;
  std::mutex list_mtx_;
  std::mutex log_mtx_;
  std::map<NodeId, MembershipEntry> membership_list_;
  bool using_suspicion_;
  int machine_;
  std::string host_;
  struct sockaddr introducer_addr_;
  socklen_t addrlen_;
  std::string port_;
  long time_stamp_;
  NodeId self_node_;
  int sock_fd_;
  bool is_introducer_;

  std::vector<NodeId> GetTargets() {
    std::vector<NodeId> retval;
    list_mtx_.lock();
    auto self_iter = membership_list_.find(self_node_);
    if (membership_list_.size() < kTargets + 2) {
      return retval;
    }
    if (self_iter == membership_list_.end()) {
      return retval;
    }
    while (true) {
      auto iter = membership_list_.begin();
      srand(time_stamp_);
      std::advance(iter, rand() % membership_list_.size());
      if (iter == self_iter) {
        continue;
      }
      if (std::find(retval.begin(), retval.end(), iter->first) !=
          retval.end()) {
        continue;
      }
      retval.push_back(iter->first);
      if (retval.size() == kTargets) {
        break;
      }
    }
    list_mtx_.unlock();
    return retval;
  }
  void Checker() {
    for (auto &pair : membership_list_)
      if (!(pair.first == self_node_)) {
        if (pair.second.local_time - membership_list_[self_node_].local_time >=
            kTFail) {
          if (using_suspicion_) {
            pair.second.status = Status::kSuspected;
            if (pair.second.local_time - membership_list_[self_node_].local_time >=
                kTFail + kTSuspect)
              pair.second.status = Status::kFailed;
            if (pair.second.local_time - membership_list_[self_node_].local_time >=
                kTFail + kTSuspect + kTCleanup)
              membership_list_.erase(pair.first);
          } else {
            pair.second.status = Status::kFailed;
            if (pair.second.local_time - membership_list_[self_node_].local_time >=
                kTFail + kTCleanup)
              membership_list_.erase(pair.first);
          }
        }
      }
    if (membership_list_[self_node_].status == Status::kSuspected)
      membership_list_[self_node_].status = Status::kAlive;
  }

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
        // do we need machine here?
        merge(other_list);
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
      merge(recvlist);
    }
    return 0;
  }
  // membership_list_: list1, other: list2
  int merge(const std::map<NodeId, MembershipEntry> &other) {
    for(auto &iter: other)
      if(iter->first!=self) {
        if (membership_list_.find(iter) == membership_list_.end()) {
            membership_list_[iter->first] = iter->second;
            membership_list_[iter->first].local_time = membership_list_[self].local_time;
            std::cout << "create new entry " << iter->first.host << std::endl;
        }
        else if (suspicion == false) {
            if (iter->second.status != kAlive && membership_list_[iter->first].status == kAlive) {
            membership_list_[iter->first].status = iter->second.status;
            std::cout << "machine " << iter->first.host << " status changed to "
                        << membership_list_[iter->first].status << std::endl;
            } else if (iter->second.count > membership_list_[iter->first].count) {
            membership_list_[iter->first].count = iter->second.count;
            membership_list_[iter->first].local_time = membership_list_[self].local_time;
            std::cout << "updated entry " << iter->first.host << " on machine " << self.host
                        << std::endl;
            } 
        }
        else {
            if (iter->second.status == kFailed || membership_list_[iter->first].status == kFailed) {
                iter->second.status = membership_list_[iter->first].status = failed;
                std::cout << "machine " << iter->first.host << " has failed!" << std::endl;
            } 
            else {
                if (iter->second.count > membership_list_[iter->first].count) {
                membership_list_[iter->first].count = iter->second.count;
                membership_list_[iter->first].local_time = membership_list_[self].local_time;
                std::cout << "updated entry " << iter->first.host << " on machine " << self.host
                            << std::endl;
                }
                if (iter->second.status != membership_list_[iter->first].status) {
                membership_list_[iter->first].status = iter->second.status;
                std::cout << "machine " << iter->first.host << " status changed to "
                            << iter->second.status << std::endl;
                }
            }
        }
    }
    return 0;
  }
  void Client() {
    sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    while (true) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(kHeartbeatInterval));
      list_mtx_.lock();
      auto iter = membership_list_.find(self_node_);
      if (iter == membership_list_.end()) {
        continue;
      }
      membership_list_[self_node_].count++;
      membership_list_[self_node_].local_time++;
      // construct package send to all neighbors
      std::string datagram;
      std::stringstream ss(datagram);
      ss << "DATA" << std::endl;
      ss << ListToString() << std::endl;
      std::vector<NodeId> targets = GetTargets();
      for (const NodeId &target : targets) {
        struct sockaddr_in service;
        memset(&service, 0, sizeof(service));
        service.sin_family = AF_INET;
        service.sin_port = htons(8080);
        service.sin_addr.s_addr = inet_addr("")
      }
      list_mtx_.unlock();
    }
  }
  void OpenClient() {
    sock_fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct addrinfo hints, *infoptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    int addr = getaddrinfo(hosts[id], "8080", &hints, &infoptr);
    if (addr) {
      perror("sender cannot set address!");
      return 1;
    }
    if (connect(sock_fd, infoptr->ai_addr, infoptr->ai_addrlen) == -1) {
      perror("sender cannot connect! target machine number = " + id);
      return 1;
    }
  }
  const std::vector<std::string> kHosts{
      "fa23-cs425-6901.cs.illinois.edu", "fa23-cs425-6902.cs.illinois.edu",
      "fa23-cs425-6903.cs.illinois.edu", "fa23-cs425-6904.cs.illinois.edu",
      "fa23-cs425-6905.cs.illinois.edu", "fa23-cs425-6906.cs.illinois.edu",
      "fa23-cs425-6907.cs.illinois.edu", "fa23-cs425-6908.cs.illinois.edu",
      "fa23-cs425-6909.cs.illinois.edu", "fa23-cs425-6910.cs.illinois.edu",
  };
};
