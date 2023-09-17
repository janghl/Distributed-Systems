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
# To test
Expectation is that there is no output, otherwise we are failing asserts and will core dump.
```bash
bash start_server.sh
make test
./test.out
bash kill_server.sh
```
