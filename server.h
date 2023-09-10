#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <iostream>

class Server {
public:
  Server() {
    // define IP address structure
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    std::cout << "Am I blocking here?" << std::endl;
    int s = getaddrinfo(NULL, "8080", &hints, &result_);
    if (s != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
      exit(1);
    }
    std::cout << "Got out of being blocked" << std::endl;
  }

  void StartServer() {
    std::cout << "trying to start server" << std::endl;;
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(server_fd_, result_->ai_addr, result_->ai_addrlen) != 0) {
      perror("bind");
      exit(1);
    }
    std::cout << "bound";
    if (listen(server_fd_, 10) != 0) {
      perror("listen");
      exit(1);
    }
    struct sockaddr_in *result_addr = (struct sockaddr_in *)result_->ai_addr;
    printf("Listening on file descriptor %d, port %d", server_fd_,
           ntohs(result_addr->sin_port));
    std::cout << std::endl;
    while (true) {
      int client_fd = accept(server_fd_, NULL, NULL);
      printf("Connection made: client_fd=%d, spinning up background thread\n",
             client_fd);
      std::thread background_thread{&Server::HandleClient, this, client_fd};
      // Detach to allow multiple client connections
      background_thread.detach();
    }
  }

  void HandleClient(int client_fd) {
    // allocate buffer memory
    char read_buffer[1024];
    char result_buffer[1024];
    memset(read_buffer, 0, sizeof(read_buffer));
    memset(result_buffer, 0, sizeof(result_buffer));

    recv(client_fd, read_buffer, 1024, 0);
    printf("Received pattern: %s\n", read_buffer);

    char grep_buffer[64];
    sprintf(grep_buffer, "grep -r --include=\\*.log %s ./", read_buffer);

    // Open 
    FILE *fp = popen(grep_buffer, "r");
    int bytes_read = 0;
    while ((bytes_read = fread(result_buffer, 1, 1024, fp)) > 0) {
      send(client_fd, result_buffer, bytes_read, 0);
    }
    close(client_fd);
  }
  int server_fd_;
  struct addrinfo *result_;
};