#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <netinet/in.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

class Server {
public:
  int server_socket;
  int bind_result;
  int client_socket;

  void Initialize() {
    // define IP address structure
    struct sockaddr_in myaddr;
    memset(&myaddr, 0, sizeof(sockaddr_in));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(8080); // host to network port transformation

    // define socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
      perror("socket cannot be created!");
      return;
    }

    // bind socket to IP address
    bind_result = bind(server_socket, (struct sockaddr *)&myaddr,
                       sizeof(struct sockaddr));
    if (bind_result < 0) {
      perror("bind failure!");
      return;
    }
  }

  void Listen() {
    // listen
    int listen_result = listen(server_socket, 10);
    if (listen_result < 0) {
      perror("cannot listen");
      return;
    } else
      std::cout << "open listening!";
  }

  void Connect() {
    // accept block
    struct sockaddr_in client_addr;
    socklen_t socket_length = sizeof(struct sockaddr);
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &socket_length);
    if (client_socket < 0) {
      perror("acceptance error!");
      return;
    }
    char *client_ip = new char[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,
              INET_ADDRSTRLEN); // network to host
    std::cout << "successfully connected to IP: " << client_ip
              << " and port: " << ntohs(client_addr.sin_port) << std::endl;

    // allocate buffer memory
    char buffer[1024];
    char send_str[1024];
    memset(buffer, '0', sizeof(buffer));
    memset(send_str, '0', sizeof(send_str));

    while (recv(client_socket, buffer, sizeof(buffer), 0) > 0) {
      std::cout << "capture a packet!\n";
      std::cout << "command = " << buffer << std::endl;

      // search the pattern and send
      std::filesystem::path pwd = std::filesystem::current_path();
      for (auto file : std::filesystem::directory_iterator(pwd)) {
        if (file.is_regular_file() && file.path().extension() == ".log") {
          std::string file_path = std::filesystem::absolute(file).string();
          std::string command = (std::string)buffer + ">temp.txt";
          int grep_result = system(command.c_str());
          if (grep_result != 0)
            std::cerr << "command fail!" << std::endl;
          std::ifstream file("temp.txt");
          while (file.read(send_str, sizeof(send_str))) {
            send(client_socket, send_str, sizeof(send_str), 0);
            memset(send_str, '0', sizeof(send_str));
          }
          if (file.gcount() > 0) {
            file.read(send_str, file.gcount());
            send(client_socket, send_str, file.gcount(), 0);
          }
          std::cout << "sent successfully to client!\nfile name: " << file_path
                    << std::endl;
        }
      }
      memset(buffer, '0', sizeof(buffer));
      continue;
    }
    std::cout << "shut down!";
    close(server_socket);
    return;
  }
};