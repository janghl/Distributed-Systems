#define MAX_CLIENTS 100
#define SERVER_REPLY_BUFFER_SIZE 512

extern int server_port;
extern bool end_session;
extern std::string machine_id;
extern std::vector<std::string> machine_ips;

int setup_server_socket();
int connect_with_server(const std::string& machine_ip);
ssize_t write_to_socket(int socket, const char* message, int message_size);
ssize_t read_from_socket(int socket, char* message, int message_size);
ssize_t read_from_socket(int socket, std::string& command);
void process_machine_names(int count, char** machine_names);
bool compare_logs(std::string expected, std::string recv);
bool save_log_to_file(const std::string &log_content, const std::string &filename);
