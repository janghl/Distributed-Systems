# Distributed Log Querier

This program runs a grep command to query log files on various virtual machines for a specific pattern.

## Details

 - This program starts TCP servers on port 8080 on boxes to be queried
 - The client handles connections to TCP servers in parallel in asynchronously using the std::async API
 - Distributed grep output includes counts of lines returned from each box, total lines returned, and total time spent on the distributed grep operation

## Usage

# Single Server
To start a server in the background on the current box:
```bash
make server
./server.out &
```
# Distributed Servers
To start servers on all assigned VMs:
```bash
bash start_server.sh
```
# Client
To query multiple machines for a specific pattern:
```bash
make client
./client.out [pattern] [num_machines]
```
# Logging
To generate logging for testing purposes
```bash
bash log.sh
```
# To clean all boxes
```bash
bash clean.sh
```
# To kill servers on all boxes
```bash
bash kill_server.sh
```
# To run the code:
```
g++ server.cpp -o a.out -std=c++17 -lstdc++fs
./a.out
```
# To clean all boxes
```

```
# To run logging:
```
g++ log.cpp -o log.out -std=c++17 -lstdc++fs
./log.out [maching number + line number]
```
