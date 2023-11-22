#pragma once

#undef FOOTER
#define FOOTER 0xdeadbeef


class Task; // forward declaration
class Job;
bool get_task_over_socket(int socket, Task& task, std::string print_statement_origin);
bool send_task_over_socket(int socket, const Task& task, std::string print_statement_origin);
bool send_job_over_socket(int socket, Job& job, std::string print_statement_origin);
