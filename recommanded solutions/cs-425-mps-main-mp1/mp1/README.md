# CS 425 - MP 1
Distributed Log Querier

## Description
Query distributed log files on multiple machine.
Our querier sends the grep command to every machine using sockets, and each machine executes the command locally and returns the result.

## Design
- each machine opens a TCP server socket over a predetermined port. When a client request arrives, the machine decodes the grep command, executes it using popen, and returns the result.
- the querier skips any machine it fails to connect to as it indicates the machine has failed. 
- once the querier has sent packets to every machine, the query waits for each machine to respond back and reports each machine's response to the console.

## Report
[report](https://docs.google.com/document/d/1JDoaT6NBLDWNf_enpUlSWtXsCpf3V-_kw6Wbzf1-07Q/edit?usp=sharing)


## Server
The server will listen for client connections. Once it accepts incoming client request, it executes the grep command. All log files are kept in the test directory inside cs425-mp-1 folder. Thus, the grep command is only asked to analyze files with log extension in cs425-mp-1/test folder. The output of grep is sent back over the socket.


## Client
Firstly, it recieves input for grep query from stdin. The querier sends the grep command as is to every server. Once the querier has sent packets to every machine, the query waits for each machine to respond back and reports each machine's response to the console. Queier also sums the number of matching lines from each server and prints that at the end.

## Unit Test

#### `test_client_server_connection`
tests whether the connection between the client and the server is established successfully and if it responds correctly.

#### `test_machine_ip_generation`
tests the correctness of the transformation from name to IP, which is used during the socket setup process.

#### `test_infrequent_pattern`
tests scenarios involving infrequent patterns.

#### `test_abrupt_connection_closed`
test the ability to manage an abrupt closure of the connection, ensuring stability and appropriate responses under such circumstances.

#### `test_failed_machine_mid_execution`
tests the scenario where a machine fails in the midst of execution, verifying the ability to handle such failures gracefully.

#### `test_machine_unreachable`
test that an unreachable machine will not be included in the responses received, maintaining the integrity of the response data.

#### `test_pattern_in_provided_logs`
tests a bunch of different patterns with various frequencies (ex. patterns appearing 0%, 20%, 40%, 60%, 80%, and 100% of times). This test runs grep command on the provided log files (from piazza). 

## Compilation
To compile program without tests: `g++ -std=c++14 main.cpp server.cpp client.cpp utils.cpp -lpthread -o server_client`

To run program without tests: `./server_client [id of the vm program is run on] [name of vm program is run on] [other vm name]..."` 

Ex. to run program on fa23-cs425-7201 with machines fa23-cs425-7201, fa23-cs425-7202, fa23-cs425-7203, execute:
`./server_client 1 fa23-cs425-7201.cs.illinois.edu fa23-cs425-7202.cs.illinois.edu fa23-cs425-7203.cs.illinois.edu`

The program expects all log files to have `.log` extenction and they must be housed in `test/` directory which itself is present in the directory from where `./server_client` was launched. The program takes input from stdin and expects `grep` to be the input prefix. Ex. to search for pattern 'hello', we will provide `grep hello` as input to stdin, to search for all lines that do not contain 'hello', we will provide `grep -v hello` as input to stdin.

## Compilation with tests
To compile tests: `g++ -std=c++14 test.cpp server.cpp client.cpp utils.cpp -lpthread -o test_server_client`

To run tests: `./test_server_client`

Note: tests assume that the name of machines are `fa23-cs425-72[machine number].cs.illinois.edu`
