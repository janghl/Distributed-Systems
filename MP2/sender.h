#pragma once

#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
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
  int controller() {
    char buffer[64];
    gethostname(buffer, 64);
    host_ = buffer;
    if (host_ == "fa23-cs425-6901.cs.illinois.edu") {
      is_introducer_ = true;
    }
    std::thread sender_thread(&Controller::Client, this);
    std::thread receiver_thread(&Controller::Receiver, this);
    std::thread checker_thread(&Controller::Checker, this);
    std::thread cli_thread(&Controller::Monitor, this);
    sender_thread.join();
    receiver_thread.join();
    checker_thread.join();
    cli_thread.join();
    return 0;
  }
  enum class Status { kAlive, kFailed, kSuspected, kLeft };
  struct NodeId {
    std::string host;
    std::string port;
    long time_stamp;
    bool operator==(const NodeId &other) const {
      return (host == other.host) && (port == other.port) &&
             (time_stamp == other.time_stamp);
    }
    bool operator<(const NodeId &other) const {
      if (host.compare(other.host) < 0) {
        return true;
      }
      if (port.compare(other.port) < 0) {
        return true;
      }
      return time_stamp < other.time_stamp;
    }
  };
  struct MembershipEntry {
    int count;
    int local_time;
    Status status;
    bool operator==(const MembershipEntry &other) const {
      return (count == other.count) && (local_time == other.local_time) &&
             (status == other.status);
    }
  };
  std::map<NodeId, MembershipEntry> membership_list_;

  void Merge(const std::map<NodeId, MembershipEntry> &other) {
    Log("Merge lock");
    list_mtx_.lock();
    for (auto &pair : other) {
      if (!(pair.first == self_node_)) {
        if (membership_list_.find(pair.first) == membership_list_.end()) {
          membership_list_[pair.first] = pair.second;
          membership_list_[pair.first].local_time =
              membership_list_[self_node_].local_time;
          Log("create new entry " + pair.first.host + "\n");
        } else if (!using_suspicion_) {
          MembershipEntry entry = membership_list_[pair.first];
          if (pair.second.status != Status::kAlive &&
              entry.status == Status::kAlive) {
            entry.status = pair.second.status;
            Log("machine " + pair.first.host + " status changed to " +
                status_to_string_[entry.status] + "\n");
          } else if (pair.second.count > membership_list_[pair.first].count) {
            membership_list_[pair.first].count = pair.second.count;
            membership_list_[pair.first].local_time =
                membership_list_[self_node_].local_time;
            Log("updated entry " + pair.first.host + " on machine " +
                self_node_.host + "\n");
          }
        } else {
          if (pair.second.status == Status::kFailed ||
              pair.second.status == Status::kLeft) {
            membership_list_[pair.first].status = pair.second.status;
            Log(+"machine " + pair.first.host + " has failed!" + "\n");
          } else {
            if (pair.second.count > membership_list_[pair.first].count) {
              membership_list_[pair.first].count = pair.second.count;
              membership_list_[pair.first].local_time =
                  membership_list_[self_node_].local_time;
              Log("updated entry " + pair.first.host + " on machine " +
                  self_node_.host + "\n");
            }
            if (pair.second.status != membership_list_[pair.first].status) {
              membership_list_[pair.first].status = pair.second.status;
              Log("machine " + pair.first.host + " status changed to " +
                  status_to_string_[pair.second.status] + "\n");
            }
          }
        }
      }
    }
    list_mtx_.unlock();
  }

  void Checker() {
    std::cout << "Started checker" << std::endl;
    while (true) {
      for (auto &pair : membership_list_)
        if (!(pair.first == self_node_)) {
          if (pair.second.local_time -
                  membership_list_[self_node_].local_time >=
              kTFail) {
            if (using_suspicion_) {
              pair.second.status = Status::kSuspected;
              if (pair.second.local_time -
                      membership_list_[self_node_].local_time >=
                  kTFail + kTSuspect)
                pair.second.status = Status::kFailed;
              if (pair.second.local_time -
                      membership_list_[self_node_].local_time >=
                  kTFail + kTSuspect + kTCleanup)
                membership_list_.erase(pair.first);
            } else {
              pair.second.status = Status::kFailed;
              if (pair.second.local_time -
                      membership_list_[self_node_].local_time >=
                  kTFail + kTCleanup)
                membership_list_.erase(pair.first);
            }
          }
        }
      if (membership_list_[self_node_].status == Status::kSuspected)
        membership_list_[self_node_].status = Status::kAlive;
    }
  }
  bool using_suspicion_;

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
    MembershipEntry entry{0, 0, Status::kAlive};
    Log("join lock");
    list_mtx_.lock();
    membership_list_[self_node_] = entry;
    Log("unlock");
    list_mtx_.unlock();
    if (is_introducer_) {
      Log("Introducer joined");
    } else {
      Log("Non-introducer joined");
      std::stringstream ss;
      ss << "JOIN" << std::endl;
      ss << host_ << " " << port_ << " " << time_stamp_ << std::endl;
      std::string message = ss.str();
      Log("Sending " + message + " to introducer");
      // Send to introducer host and port
      struct addrinfo hints, *infoptr;
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_flags = AI_PASSIVE;
      int s = getaddrinfo("fa23-cs425-6901.cs.illinois.edu", "8080", &hints,
                          &infoptr);
      if (s) {
        perror("receiver cannot set address!");
        return;
      }
      sendto(sock_fd_, message.c_str(), message.length(), 0, infoptr->ai_addr,
             infoptr->ai_addrlen);
    }

    // cond var to make sure the fd is connected
    // then mutex lock around fd
    // then mutex unlock
    // send message to introducer to help join
  }
  void Leave() {
    Log("leave lock");
    list_mtx_.lock();
    std::cout << host_ << port_ << time_stamp_ << std::endl;
    auto it = membership_list_.find(NodeId{host_, port_, time_stamp_});
    if (it == membership_list_.end()) {
      Log("Couldn't find self entry in membership list");
      Log("unlock");
      list_mtx_.unlock();
      return;
    }
    if (it->second.status != Status::kAlive) {
      Log("Only alive node can voluntarily leave");
      Log("unlock");
      list_mtx_.unlock();
      return;
    }
    it->second.status = Status::kLeft;
    Log("unlock");
    list_mtx_.unlock();
  }
  void List() {
    Log("Membership List on host " + host_ + "\n");
    Log("list lock");
    list_mtx_.lock();
    for (const auto &[key, entry] : membership_list_) {
      std::string status_string = status_to_string_[entry.status];
      printf("Host: %s, count: %d, local_time: %d, status: %s",
             key.host.c_str(), entry.count, entry.local_time,
             status_string.c_str());
      std::cout << std::endl;
    }
    Log("unlock");
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

  std::string ListToString() {
    Log("tostring lock");
    list_mtx_.lock();
    std::stringstream ss;
    for (const auto &[key, value] : membership_list_) {
      ss << key.host << "," << key.port << "," << key.time_stamp << ","
         << value.count << "," << value.local_time << ","
         << status_to_string_.at(value.status) << " ";
    }
    ss << std::endl;
    Log("unlock");
    list_mtx_.unlock();
    return ss.str();
  }
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
  std::map<NodeId, MembershipEntry> StringToList(const std::string &str) const {
    std::map<NodeId, MembershipEntry> map;
    std::string entry;
    while (std::stringstream(str) >> entry) {
      std::vector<std::string> splitted;
      std::string substr;
      while (getline(std::stringstream(entry), substr, ',')) {
        splitted.push_back(substr);
      }
      NodeId node{splitted[0], splitted[1], std::stol(splitted[2])};
      MembershipEntry entry{std::stoi(splitted[3]), std::stoi(splitted[4]),
                            string_to_status_.at(splitted[5])};
      map[node] = entry;
    }
    return map;
  }
  const int kTFail = 30;
  const int kTCleanup = 20;
  const int kTSuspect = 20;
  const int kHeartbeatInterval = 20;
  const int kScale = 10;
  const unsigned int kTargets = 2;
  std::mutex list_mtx_;
  std::mutex log_mtx_;
  std::string host_;
  struct sockaddr introducer_addr_;
  socklen_t addrlen_;
  std::string port_ = "8080";
  long time_stamp_;
  NodeId self_node_;
  int sock_fd_;
  bool is_introducer_;

  std::vector<NodeId> GetTargets() {
    std::vector<NodeId> retval;
    Log("targets lock");
    list_mtx_.lock();
    auto self_iter = membership_list_.find(self_node_);
    if (membership_list_.size() == 1) {
      Log("unlock");
      list_mtx_.unlock();
      return retval;
    }
    if (self_iter == membership_list_.end()) {
      Log("unlock");
      list_mtx_.unlock();
      return retval;
    }
    while (true) {
      auto iter = membership_list_.begin();
      srand(time_stamp_);
      std::advance(iter, rand() % membership_list_.size());
      if (iter == self_iter) {
        continue;
      }
      retval.push_back(iter->first);
      if (retval.size() == kTargets ||
          retval.size() >= membership_list_.size() - 1) {
        break;
      }
    }
    Log("unlock");
    list_mtx_.unlock();
    return retval;
  }

  void Receiver() {
    std::cout << "Started receiver" << std::endl;
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct addrinfo hints, *infoptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    int s = getaddrinfo(NULL, "8080", &hints, &infoptr);
    if (s) {
      perror("receiver cannot set address!");
      return;
    }
    if (bind(sock_fd, infoptr->ai_addr, infoptr->ai_addrlen) == -1) {
      perror("receiver cannot bind!");
      return;
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
      int client_len;
      struct sockaddr_in client_addr;
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
        Merge(other_list);
      } else if (is_introducer_) {
        std::string host;
        std::string port;
        long time_stamp;
        ss >> host >> port >> time_stamp;
        NodeId node{host, port, time_stamp};
        MembershipEntry entry{0, 0, Status::kAlive};
        Log("receiver lock");
        list_mtx_.lock();
        membership_list_[node] = entry;
        Log("unlock");
        list_mtx_.unlock();
        std::stringstream oss;
        oss << "DATA" << std::endl;
        oss << ListToString();
        std::string data = oss.str();
        // Send membership list back to newly joined member
        sendto(sock_fd_, data.c_str(), data.size(), 0,
               (struct sockaddr *)&client_addr, addrlen);
      }
    }
  }
  // membership_list_: list1, other: list2
  void Client() {
    std::cout << "Started client" << std::endl;
    sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    while (true) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(kHeartbeatInterval));
      Log("client lock");
      list_mtx_.lock();
      auto iter = membership_list_.find(self_node_);
      if (iter == membership_list_.end()) {
        continue;
      }
      membership_list_[self_node_].count++;
      membership_list_[self_node_].local_time++;
      Log("unlock");
      list_mtx_.unlock();
      // construct package send to all neighbors
      std::stringstream ss;
      ss << "DATA" << std::endl;
      ss << ListToString() << std::endl;
      std::string datagram = ss.str();
      std::vector<NodeId> targets = GetTargets();
      for (const NodeId &target : targets) {
        Log("sending datagram");
        struct addrinfo hints, *infoptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;
        int s = getaddrinfo(target.host.c_str(), target.port.c_str(), &hints,
                            &infoptr);
        if (s) {
          perror("receiver cannot set address!");
          return;
        }
        sendto(sock_fd_, datagram.c_str(), datagram.size(), 0, infoptr->ai_addr,
               infoptr->ai_addrlen);
      }
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
