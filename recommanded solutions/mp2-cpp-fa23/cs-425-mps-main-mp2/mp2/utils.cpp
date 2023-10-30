#include <arpa/inet.h>

#include <cstring>
#include <chrono>
#include <mutex>
#include <iostream>
#include <fstream>
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
#include "membership_list_manager.hpp"
#include "../mp1/utils.hpp"

#define HASH_A 65689
#define MAX_BUFFER_SIZE 1300
#define MAGIC 0xdeadbeef

// Returns the first ip address it finds for machine with name "fa23-cs425-72[machine_id].cs.illinois.edu"
std::string get_machine_ip(uint32_t machine_id) {
	std::string machine_name = get_machine_name(machine_id);
	struct addrinfo hints, *res, *p;
	int status;
	char ipstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	status = getaddrinfo(machine_name.c_str(), NULL, &hints, &res);
	if (status != 0) {
		std::cerr << "getaddrinfo from get_machine_ip for " << machine_name << " (machine id " << machine_id << "): " 
			<< gai_strerror(status) << std::endl;
		return "";
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
		freeaddrinfo(res);
		return ipstr;
	}
	freeaddrinfo(res);
	return "";
}

// Returns "fa23-cs425-72[machine_id formatted as a string of length 2].cs.illinois.edu"
std::string get_machine_name(uint32_t machine_id) {
	std::string machine_id_str = std::to_string(machine_id);
	if (machine_id_str.length() < 2) {
		machine_id_str = "0" + machine_id_str;
	} else if (machine_id_str.length() > 2) {
		std::cerr << "Invalid machine number: " << machine_id << "(" << machine_id_str <<
		       		"); machine number has more than 2 digits" << std::endl;
		exit(-1);
	}
	return "fa23-cs425-72" + machine_id_str + ".cs.illinois.edu";
}


// Returns current time since epoch in period interval
unsigned long get_current_time() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}



// Open socket at server_port. Socket is TCP if socket_type is set to SOCK_STREAM and UDP if socket_type is SOCK_DGRAM. 
// Sets some other socket properties as well, namely: AF_INET, AI_PASSIVE, SO_REUSEPORT and SO_REUSEADDR.
// Also, binds socket to port and calls listen on socket. Returns socket fd which can be used by accept to recieve incoming client requests.
int setup_server_socket(int server_port, int socket_type, int protocol, int socket_domain) {
	int sock_fd = socket(socket_domain, socket_type, 0);
	if (sock_fd < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = socket_type;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = protocol;
	int getAddrErr = getaddrinfo(NULL, std::to_string(server_port).c_str(), &hints, &result);
	if (getAddrErr != 0) {
		std::cerr << gai_strerror(getAddrErr) << std::endl;
		exit(EXIT_FAILURE);
	}

	int optval = 1;
	int socketopt = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	if (socketopt < 0) {
		printf("set socket opt %d:", server_port);
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	socketopt = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (socketopt < 0) {
		printf("set socket opt %d:", server_port);
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	int bindErr = bind(sock_fd, result->ai_addr, result->ai_addrlen);
	if (bindErr != 0) {
		printf("bind %d:", server_port);
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);

	return sock_fd;
}

// Open TCP socket at port. Sets socket properties: AF_INET, AI_PASSIVE, SO_REUSEPORT and SO_REUSEADDR.
// Also, binds socket to port and calls listen on socket. Returns socket fd which can be used by accept to recieve incoming client requests.
int setup_tcp_server_socket(int server_port) {
	int sock_fd = setup_server_socket(server_port, SOCK_STREAM, IPPROTO_TCP, AF_INET);

	int listenErr = listen(sock_fd, MAX_CLIENTS);
        if (listenErr != 0) {
                perror(NULL);
                exit(EXIT_FAILURE);
        }
	return sock_fd;
}

// Open UDP socket at port. Sets socket properties: AF_INET, AI_PASSIVE, SO_REUSEPORT and SO_REUSEADDR.
// Also, binds socket to port and calls listen on socket. Returns socket fd which can be used by accept to recieve incoming client requests.
int setup_udp_server_socket(int server_port) {
	return setup_server_socket(server_port, SOCK_DGRAM, IPPROTO_UDP, AF_INET);
}


// Opens a connection with machine_ip (ip address) at port server_port. On success, returns a socket fd which can be used to communicate 
// with server using read/write. On failure, returns -1.
int setup_client_socket(const std::string& machine_ip, int server_port, int socket_type, int socket_domain) {
	int sock_fd = socket(socket_domain, socket_type, 0);

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
		std::cerr << "Connection with " << machine_ip << ":" << server_port << " failed" << std::endl;
		perror("Connect error");
		return -1;
	}

	return sock_fd;
}

// Open UDP socket and connects with machine_ip at machine_port.
// Returns socket fd which can be used to send/get data to/from server using read/write calls respectively.
int setup_udp_client_socket(const std::string& machine_ip, int server_port) {
	return setup_client_socket(machine_ip, server_port, SOCK_DGRAM, AF_INET);
}

// Open TCP socket and connects with machine_ip at machine_port.
// Returns socket fd which can be used to send/get data to/from server using read/write calls respectively.
int setup_tcp_client_socket(const std::string& machine_ip, int server_port) {
	return setup_client_socket(machine_ip, server_port, SOCK_STREAM, AF_INET);
}

void log(const char* message, const char* message_type, std::string file_path) {
	std::ofstream logfile;
	logfile.open(file_path, std::ios_base::app);

	std::time_t current_time = std::time(nullptr);
	std::string current_time_str = std::ctime(&current_time);
	current_time_str.erase(current_time_str.length() - 1); // remove new line character
	logfile << current_time_str << " - [" << message_type << "] - " << message << std::endl;

	logfile.close();
}

void log_info(const char* message, std::string file_path) {
	log(message, "INFO", file_path);
}

void log_error(const char* message, std::string file_path) {
	log(message, "ERROR", file_path);
}

void log_warning(const char* message, std::string file_path) {
	log(message, "WARNING", file_path);
}

void log_debug(const char* message, std::string file_path) {
	log(message, "DEBUG", file_path);
}

std::string stringify_machine_id_version(uint32_t machine_id, uint32_t machine_version) {
	return std::to_string(machine_id) + "-" + std::to_string(machine_version);
}

void log_machine_suspected(uint32_t machine_id, uint32_t machine_version) {
	std::string message = "SUSPECTED: Machine " + stringify_machine_id_version(machine_id, machine_version) + " is suspected";
	log_info(message.c_str(), "test/mp2_logs.log");
}

void log_machine_failed(uint32_t machine_id, uint32_t machine_version) {
	std::string message = "FAILED: Machine " + stringify_machine_id_version(machine_id, machine_version) + " has failed";
	log_info(message.c_str(), "test/mp2_logs.log");
}

void log_machine_left(uint32_t machine_id, uint32_t machine_version) {
        std::string message = "LEAVE: Machine " + stringify_machine_id_version(machine_id, machine_version) + " has failed";
        log_info(message.c_str(), "test/mp2_logs.log");
}

void log_machine_purged(uint32_t machine_id, uint32_t machine_version) {
	std::string message = "PURGED: Machine " + stringify_machine_id_version(machine_id, machine_version) + 
		" has failed and is removed from membership list";
	log_info(message.c_str(), "test/mp2_logs.log");
}

void log_machine_new_node(uint32_t machine_id, uint32_t machine_version) {
	std::string message = "JOINED: Machine " + stringify_machine_id_version(machine_id, machine_version) + " has joined the group";
	log_info(message.c_str(), "test/mp2_logs.log");
}

void log_thread_started(std::string thread_info) {
	std::string message = "STARTING: " + thread_info;
        log_info(message.c_str(), "test/mp2_logs.log");
}

void send_membership_list(int socket, const char* serialized_membership_list, uint32_t membership_list_size, uint32_t& hash_value, 
		uint32_t& hash_a, char*& output_buffer) {
	// Send the size of membership list first
	uint32_t network_membership_list_size = htonl(membership_list_size);
	memcpy(output_buffer, (char*)&network_membership_list_size, sizeof(uint32_t));
	hash_message((char*)&membership_list_size, sizeof(uint32_t), hash_value, hash_a);
	output_buffer += sizeof(uint32_t);

	// Send the membership list itself
	memcpy(output_buffer, serialized_membership_list, membership_list_size);
	hash_message(serialized_membership_list, membership_list_size, hash_value, hash_a);
	output_buffer += membership_list_size;
}

void send_membership_and_protocol(int socket, uint32_t self_id, uint32_t self_version, const char* serialized_membership_list, 
		uint32_t membership_list_size) {
	// client message has form: 
	// [size of message][magic][client id][client version][generation count][gossip protocol][payload size][payload][checksum]
	uint32_t hash_value = 0;
	uint32_t hash_a = HASH_A;

	uint32_t total_packet_size = membership_list_size + (sizeof(uint32_t) * 8);
	assert(total_packet_size < MAX_BUFFER_SIZE);
	bytes_sent += total_packet_size;

	char* buffer = new char[total_packet_size];
	char* buffer_start = buffer;

	uint32_t network_total_packet_size = htonl(total_packet_size);
	memcpy(buffer, (char*)&network_total_packet_size, sizeof(uint32_t));
	hash_message((char*)&total_packet_size, sizeof(uint32_t), hash_value, hash_a);
	buffer += sizeof(uint32_t);

	uint32_t magic = MAGIC;
	uint32_t network_magic = htonl(MAGIC);
	memcpy(buffer, (char*)&network_magic, sizeof(uint32_t));
	hash_message((char*)&magic, sizeof(uint32_t), hash_value, hash_a);
	buffer += sizeof(uint32_t);

	// Send self id
	uint32_t network_self_id = htonl((uint32_t)self_id);
	memcpy(buffer, (char*)&network_self_id, sizeof(uint32_t));
	hash_message((char*)&self_id, sizeof(uint32_t), hash_value, hash_a);
	buffer += sizeof(uint32_t);

	// Send self version
	uint32_t network_self_version = htonl((uint32_t)self_version);
	memcpy(buffer, (char*)&network_self_version, sizeof(uint32_t));
	hash_message((char*)&self_version, sizeof(uint32_t), hash_value, hash_a);
	buffer += sizeof(uint32_t);

	protocol_mutex.lock();
	// Send generation count
	uint32_t network_generation_count = htonl(generation_count);
	memcpy(buffer, (char*)&network_generation_count, sizeof(uint32_t));
	hash_message((char*)&generation_count, sizeof(uint32_t), hash_value, hash_a);
	buffer += sizeof(uint32_t);

	// Send gossip protocol
	uint32_t gossip_protocol = (uint32_t)membership_protocol;
	uint32_t network_gossip_protocol = htonl(gossip_protocol);
	memcpy(buffer, (char*)&network_gossip_protocol, sizeof(uint32_t));
	hash_message((char*)&gossip_protocol, sizeof(uint32_t), hash_value, hash_a);
	buffer += sizeof(uint32_t);

	protocol_mutex.unlock();

	// Send membership list
	send_membership_list(socket, serialized_membership_list, membership_list_size, hash_value, hash_a, buffer);

	// Send checksum
	uint32_t network_hash = htonl(hash_value);
        memcpy(buffer, (char*)&network_hash, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

#ifdef DEBUG
	std::cout << "buffer: " << buffer << " buffer start: " << buffer_start << " total packet size: " << total_packet_size << std::endl;
#endif

	send(socket, buffer_start, total_packet_size, 0);
	delete[] buffer_start;
	buffer_start = nullptr;
	buffer = nullptr;

#ifdef DEBUG
	std::cout << "Sending message " << self_id << ":" << self_version << "(" << hash_value << ")" << std::endl;
	std::cout << "total size: " << total_packet_size << " and packet size: " << membership_list_size << std::endl;
#endif
}

bool get_membership_and_protocol(int socket, uint32_t& client_generation_count, membership_protocol_options& client_gossip_protocol, 
					std::vector<MemberInformation>& client_membership_list) {
	// client message has form: 
	// [size of message][magic][client id][client version][generation count][gossip protocol][payload size][payload][checksum]
	// everything except payload is uint32_t and payload is a char buffer of size payload size
	uint32_t hash_value = 0;
	uint32_t hash_a = HASH_A;

#ifndef TCP
	char* buffer = new char[MAX_BUFFER_SIZE];
	char* org_buffer = buffer;
	if (recv(socket, buffer, MAX_BUFFER_SIZE, 0) < (sizeof(uint32_t) * 8)) {
#ifdef DEBUG
		std::cout << "Client sent incomplete packet" << std::endl;
#endif
	}
#endif

	// Decode message size
        uint32_t client_total_message_size = 0;
#ifdef TCP
        if (read_from_socket(socket, (char*)&client_total_message_size, sizeof(uint32_t)) != sizeof(uint32_t)) {
#ifdef DEBUG
                std::cout << "Client did not send total message size" << std::endl;
#endif
                return false;
        }
#else // UDP
        memcpy((char*)&client_total_message_size, buffer, sizeof(uint32_t));
        buffer += sizeof(uint32_t);
#endif
        client_total_message_size = (uint32_t)ntohl(client_total_message_size);
	bytes_received += client_total_message_size;
        hash_message((char*)&client_total_message_size, sizeof(uint32_t), hash_value, hash_a);

	// Decode magic
	uint32_t client_magic = 0;
#ifdef TPCP
	if (read_from_socket(socket, (char*)&client_magic, sizeof(uint32_t)) != sizeof(uint32_t)) {
#ifdef DEBUG
                std::cout << "Client did not send magic" << std::endl;
#endif
                return false;
        }
#else // UCP
	memcpy((char*)&client_magic, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
#endif
        client_magic = (uint32_t)ntohl(client_magic);

	// Verify magic matches
	if (client_magic != MAGIC) {
#ifdef DEBUG
                std::cout << "Invalid magic: got " << client_magic << " but expected " << MAGIC << std::endl;
#endif
		return false;
	}
	hash_message((char*)&client_magic, sizeof(uint32_t), hash_value, hash_a);

#ifdef TCP
	// Read entire message 
	char* buffer = new char[client_total_message_size];
	char* org_buffer = buffer;
	if (read_from_socket(socket, buffer, client_total_message_size) != client_total_message_size) {
#ifdef DEBUG
                std::cout << "Invalid message: got message of size less than " << client_total_message_size << std::endl;
#endif
                return false;
        }
#endif

	// Decode client id 
	uint32_t client_id;
	memcpy((char*)&client_id, buffer, sizeof(uint32_t));
        client_id = (uint32_t)ntohl(client_id);
        buffer += sizeof(uint32_t);
	hash_message((char*)&client_id, sizeof(uint32_t), hash_value, hash_a);

	// Decode client version 
	uint32_t client_version;
	memcpy((char*)&client_version, buffer, sizeof(uint32_t));
        client_version = (uint32_t)ntohl(client_version);
        buffer += sizeof(uint32_t);
	hash_message((char*)&client_version, sizeof(uint32_t), hash_value, hash_a);

#ifdef DEBUG
	std::cout << "Recieved message from: "  << client_id << ":" << client_version << std::endl;
#endif
	
	// Decode generation count
	memcpy((char*)&client_generation_count, buffer, sizeof(uint32_t));
	client_generation_count = (uint32_t)ntohl(client_generation_count);
	buffer += sizeof(uint32_t);
	hash_message((char*)&client_generation_count, sizeof(uint32_t), hash_value, hash_a);

	// Decode gossip protocol
	uint32_t client_gossip_protocol_int;
	memcpy((char*)&client_gossip_protocol_int, buffer, sizeof(uint32_t));
	client_gossip_protocol_int = (uint32_t)ntohl(client_gossip_protocol_int);
	client_gossip_protocol = static_cast<membership_protocol_options>(client_gossip_protocol_int);
	buffer += sizeof(uint32_t);
	hash_message((char*)&client_gossip_protocol_int, sizeof(uint32_t), hash_value, hash_a);

	// Decode membership list size (aka payload size)
	uint32_t payload_size;
        memcpy((char*)&payload_size, buffer, sizeof(uint32_t));
        payload_size = (uint32_t)ntohl(payload_size);
        buffer += sizeof(uint32_t);
	hash_message((char*)&payload_size, sizeof(uint32_t), hash_value, hash_a);

	bool fail = false;
	if (payload_size != (client_total_message_size - (sizeof(uint32_t) * 8))) {
		fail = true;
#ifdef DEBUG
		std::cout << "payload size mismatch: payload size: " << payload_size << " total size: " << client_total_message_size;
		std::cout << " expected payload size: " << (client_total_message_size - (sizeof(uint32_t) * 8)) << std::endl;
#endif

		payload_size = (client_total_message_size - (sizeof(uint32_t) * 8));
	}

	// Decode membership list (aka payload)
	hash_message(buffer, payload_size, hash_value, hash_a);
	client_membership_list = MembershipListManager::deserialize_membership_list(buffer, payload_size, 
			client_gossip_protocol == membership_protocol_options::GOSSIP_S);

	// Decode checksum
	buffer += payload_size;
	uint32_t checksum;
        memcpy((char*)&checksum, buffer, sizeof(uint32_t));
	checksum = (uint32_t)ntohl(checksum);

#ifdef DEBUG
	if (hash_value != checksum) {
		std::cout << "checksum mismatch: calculated: " << hash_value << " vs expected: " << checksum << std::endl;
		assert(hash_value == checksum);
	} else if (fail) {
		std::cout << "checksums matched if payload size is set to " << payload_size << std::endl;
	}
	assert(not fail);
#endif

        delete[] org_buffer;
	org_buffer = nullptr;
	buffer = nullptr;
        return true;
}

bool NOT_USED_get_membership_list_from_socket(int socket, std::vector<MemberInformation>& client_membership_list, bool deserialize_incarnation) {
	// Decode client message
        // client message has form: [size of payload - type uint32_t] [payload - type char*]
        uint32_t payload_size = 0;
        if (read_from_socket(socket, (char*)&payload_size, sizeof(uint32_t)) != sizeof(uint32_t)) {
		return false;
	}
        payload_size = (uint32_t)ntohl(payload_size);

        char* payload = new char[payload_size];
        if (read_from_socket(socket, payload, payload_size) != payload_size) {
		delete payload;
		return false;
	}

        client_membership_list = MembershipListManager::deserialize_membership_list(payload, payload_size, deserialize_incarnation);

	delete payload;
	return true;
}

bool NOT_USED_get_generation_count_and_protocol_from_socket(int socket, uint32_t& client_generation_count, 
		membership_protocol_options& client_gossip_protocol) { 
	// socket will write [generation count - 4 bytes][gossip protocol - 4 bytes - abides by membership_protocol_options enum]
        client_generation_count = 0;
        if (read(socket, (char*)&client_generation_count, sizeof(uint32_t)) != sizeof(uint32_t)) {
                return false;
        }
        client_generation_count = (uint32_t)ntohl(client_generation_count);

        uint32_t client_gossip_protocol_int = 0;
        if (read(socket, (char*)&client_gossip_protocol_int, sizeof(uint32_t)) != sizeof(uint32_t)) {
                return false;
        }
        client_gossip_protocol_int = (uint32_t)ntohl(client_gossip_protocol_int);

	client_gossip_protocol = static_cast<membership_protocol_options>(client_gossip_protocol_int);
	return true;
}

void get_gossip_message(int socket, bool drop_message) {
        uint32_t client_generation_count;
        membership_protocol_options client_gossip_protocol;
        std::vector<MemberInformation> client_membership_list;
        if (not get_membership_and_protocol(socket, client_generation_count, client_gossip_protocol, client_membership_list)) {
                return;
        }

        if (drop_message) {
                return;
        }

        // Check if the gossip protocol has been updated
        protocol_mutex.lock();
        if (generation_count < client_generation_count) {
                membership_protocol = client_gossip_protocol;
                generation_count = client_generation_count;
        }
        protocol_mutex.unlock();

        // Update local membership list using client's message
        membership_list_manager.merge_membership_list(client_membership_list);

#ifdef DEBUG
        std::cout << "merged membership list: " << std::endl;
        membership_list_manager.print(false);
#endif

        // Check if machine running this process is suspected; if yes, update incarnation number and set status back to alive
        if (membership_list_manager.self_is_suspected()) {
                membership_list_manager.self_increment_incarnation_number();
        }
}

bool is_protocol_gossip() {
	protocol_mutex.lock();
	bool return_val = membership_protocol == membership_protocol_options::GOSSIP;
	protocol_mutex.unlock();
	return return_val;
}

// For each machine in machine_names, finds the ip address of the machine and appends it to machine_ips vector (global declared in utils.hpp)
void process_machine_names(const std::vector<std::string>& machine_names) {
	for (const std::string& machine_name : machine_names) {
		struct addrinfo hints, *res, *p;
		int status;
		char ipstr[INET6_ADDRSTRLEN];

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		status = getaddrinfo(machine_name.c_str(), NULL, &hints, &res);
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
		freeaddrinfo(res);
    }
}

void hash_message(const char* message, uint32_t message_length, uint32_t& hash, uint32_t& a) {
	static constexpr uint32_t b = 378551;

	for (uint32_t i = 0; i < message_length; ++i) {
		hash = (hash * a) + message[i];
		a *= b;
	}
}
