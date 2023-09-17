#pragma once

#include <cstdio>
#include <cstring>
#include <fstream>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

namespace client_utils {

const static char *hosts[] = {
    "fa23-cs425-6901.cs.illinois.edu", "fa23-cs425-6902.cs.illinois.edu",
    "fa23-cs425-6903.cs.illinois.edu", "fa23-cs425-6904.cs.illinois.edu",
    "fa23-cs425-6905.cs.illinois.edu", "fa23-cs425-6906.cs.illinois.edu",
    "fa23-cs425-6907.cs.illinois.edu", "fa23-cs425-6908.cs.illinois.edu",
    "fa23-cs425-6909.cs.illinois.edu", "fa23-cs425-6910.cs.illinois.edu",
};

/**
 * Opens a TCP client that queries a server for grep output and writes it to a
 * file for the main thread to read.
 * Returns 1 on failure and 0 on success
 */
inline int QueryServer(const char *pattern, int id) {
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

  struct addrinfo hints, *infoptr;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  int result = getaddrinfo(hosts[id], "8080", &hints, &infoptr);
  if (result) {
    fprintf(stderr, "getaddrinfo: %s from thread %d\n", gai_strerror(result),
            id);
    return 1;
  }

  if (connect(sock_fd, infoptr->ai_addr, infoptr->ai_addrlen) == -1) {
    perror("connect");
    return 1;
  }

  write(sock_fd, pattern, strlen(pattern));

  std::ofstream file;
  std::string file_name = std::to_string(id + 1) + ".temp";
  file.open(file_name);

  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  while (read(sock_fd, buffer, 1024)) {
    file << buffer;
    memset(buffer, 0, sizeof(buffer));
  }
  return 0;
}

} // namespace client_utils