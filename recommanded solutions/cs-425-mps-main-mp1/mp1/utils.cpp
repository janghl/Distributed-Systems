#include <arpa/inet.h>

#include <cstring>
#include <iostream>
#include <netdb.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <netdb.h>
#include <arpa/inet.h>
#include <fstream>

#include "utils.hpp"

// Open TCP socket at server_port (global declared in utils.hpp). Sets socket properties: AF_INET, AI_PASSIVE, SO_REUSEPORT and SO_REUSEADDR.
// Also, binds socket to port and calls listen on socket. Returns socket fd which can be used by accept to recieve incoming client requests. 
int setup_server_socket() {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	int getAddrErr = getaddrinfo(NULL, std::to_string(server_port).c_str(), &hints, &result);
	if (getAddrErr != 0) {
		std::cerr << gai_strerror(getAddrErr) << std::endl;
		exit(EXIT_FAILURE);
	}

	int optval = 1;
	int socketopt = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	if (socketopt < 0) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	socketopt = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (socketopt < 0) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	int bindErr = bind(sock_fd, result->ai_addr, result->ai_addrlen);
	if (bindErr != 0) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	int listenErr = listen(sock_fd, MAX_CLIENTS);
	if (listenErr != 0) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);

	return sock_fd;
}

// Opens a connection with machine_ip (ip address) at port server_port (global declared in utils.hpp). On success, returns a socket fd which
// can be used to communicate with server using read/write. On failure, returns -1.
int connect_with_server(const std::string& machine_ip) {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));

	server_address.sin_family = AF_INET;
	int port_num = server_port;
	server_address.sin_port = htons(port_num);

	if (inet_pton(AF_INET, machine_ip.c_str(), &server_address.sin_addr) <= 0) {
		std::cerr << "Invalid host ip address and/or port" << std::endl;
		return -1;
	}

	int connectErr = connect(sock_fd, (struct sockaddr*)&server_address, sizeof(server_address));
	if (connectErr != 0) {
		// perror(NULL);
		return -1;
	}

	return sock_fd;
}

// Writes message_size bytes from message to socket. On success, returns number of bytes it actually wrote. On failure/error, returns -1.
ssize_t write_to_socket(int socket, const char* message, int message_size) {
	ssize_t bytes_sent = 0;
	while (bytes_sent < message_size) {
		errno = 0;
		ssize_t nbytes = write(socket, message + bytes_sent, message_size - bytes_sent);
		if (nbytes > 0) {
			bytes_sent += nbytes;
			continue;
		}
		if (errno == EAGAIN || errno == EINTR) {
			continue;
		}
		return -1; // error
	}
	return bytes_sent;
}

// Reads message_size bytes from socket and stores them in message buffer (message buffer size must be at least message_size). On success,
// returns number of bytes it actually read. On failure/error, returns -1.
ssize_t read_from_socket(int socket, char* message, int message_size) {
        ssize_t bytes_sent = 0;
        while (bytes_sent < message_size) {
                errno = 0;
                ssize_t nbytes = read(socket, message + bytes_sent, message_size - bytes_sent);
                if (nbytes > 0) {
                        bytes_sent += nbytes;
                        continue;
                }
		if (nbytes == 0) { // EOF found
			break;
		}
                if (errno == EAGAIN || errno == EINTR) {
                        continue;
                }
                return -1; // error
        }
        return bytes_sent;
}

// Reads from socket until it encounters EOF or error. The result is returned as a string via command argument.
ssize_t read_from_socket(int socket, std::string& command) {
	ssize_t bytes_read = 0;
	char buffer[256];
	while (true) {
		buffer[0] = '\0';
		ssize_t nbytes = read_from_socket(socket, buffer, sizeof(buffer));
		if(nbytes > 0){
			bytes_read += nbytes;
			command.append(buffer, nbytes);
			continue;
		}
		if(nbytes == 0){
			break;
		}
		return -1;
	}
	return bytes_read;
}

// For each machine in machine_names, finds the ip address of the machine and appends it to machine_ips vector (global declared in utils.hpp)
void process_machine_names(int count, char** machine_names) {
	for (int i = 0; i < count; ++i) {
		struct addrinfo hints, *res, *p;
		int status;
		char ipstr[INET6_ADDRSTRLEN];

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC; 
		hints.ai_socktype = SOCK_STREAM;

		status = getaddrinfo(machine_names[i], NULL, &hints, &res);
		if (status != 0) {
			printf("getaddrinfo: %s\n", gai_strerror(status));
			continue;
		}
		// Iterate through all the addresses getaddrinfo returned and add each of them, add their ip into machine_ipds vector
		for (p = res; p != NULL; p = p->ai_next) {
			void *addr;
			if (p->ai_family == AF_INET) {
				struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
				addr = &(ipv4->sin_addr);
			} else {
				struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
				addr = &(ipv6->sin6_addr);
			}

			inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
			machine_ips.push_back(ipstr);
			
		}
    
    }
}



// Creates a file by name filename and stores log_content into the file
bool save_log_to_file(const std::string &log_content, const std::string &filename){
	std::ofstream logfile(filename);
    if (logfile.is_open()) {
        logfile << log_content;
        logfile.close();
        std::cout << "Log saved to: " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Error opening log file for writing" << std::endl;
        return false;
    }
	return true;
}

// Compares the contents of two files. As arguments, takes the name to the two files whose contents are to be compared. Returns true if 
// the files are an exact replica of each other (i.e. all content is the same AND all content appears in the same order)
bool compare_log_file(std::string expected, std::string recv){
	std::ifstream expected_file(expected);
    std::ifstream recv_file(recv);

    if (!expected_file.is_open()) {
        std::cerr << "Error opening expected log files " << std::endl;
        return false;
    }
	if(!recv_file.is_open()) {
		std::cerr << "Error opening received log files " << std::endl;
        return false;
	}

    std::string expected_line, recv_line;
    bool logs_equal = true;

    while (std::getline(expected_file, expected_line) && std::getline(recv_file, recv_line)) {
        if (expected_line != recv_line) {
            logs_equal = false;
            break;
        }
    }

    expected_file.close();
    recv_file.close();

    if (logs_equal) {
        std::cout << "Log files are equal." << std::endl;
        return true;
    } else {
		std::cout << "Log files are different." << std::endl;
        return false;
    }
}
	

