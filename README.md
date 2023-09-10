# Distributed Log Querier

This program runs a grep command to query log files on various virtual machines for a specific pattern.

## Usage

# Single Server
To start a server in the background on the current box:
```
make server
./server.out &
```
# Distributed Servers
To start servers on all assigned VMs:
```
bash start_server.sh
```
# Client
To query multiple machines for a specific pattern:
```
make client
./client.out [pattern] [num_machines]
```
# Logging
To generate logging for testing purposes
```
bash log.sh
```
# To clean all boxes
```
bash clean.sh
```
# To kill servers on all boxes
```
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
