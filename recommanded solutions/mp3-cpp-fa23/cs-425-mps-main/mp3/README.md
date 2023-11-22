# CS 425 - MP 3
Simple Distributed File System

## Description
Design for a basic file system across a distributed network of nodes, using a namenode and multiple datanodes. 

## Design
#### Namenode:
Manages block reports and orchestration of file operations across datanodes.

#### Datanode:
Store file data and metadata, participating in gossip protocol for state consistency.

#### Operation Scheduling: 
Requests are separated into FIFO queues, maintained per file. 
We track the number of consecutive read and write requests that have been executed in the immediate past to prevent opreation starvation. 

#### Failure Recovery:
Replica repairs for datanodes and leader election for namenode failure.

## Report
[report](https://docs.google.com/document/d/1x_fG6RzSCi5FJoAmaPBi3-DhbM4KIn9ilJitDkYNGFI/edit?usp=sharing)

## Compilation
Compile program: 
`./compile.sh`


## Execution
- Run program: `./sdfs [options]`
- Options: 
    - -v [current machine version]
    - -s [current machine name]
    - -d [message drop rate]
    - -o [other machine names, space separated]
    - Optional :
        - -p [protocol - GOSSIP or GOSSIP_S - defaults to GOSSIP]
        - -I [introducer server port]
        - -G [gossip server port]
        - -R [grep server port]
        - -D [data node server port]
        - -N [name node server port]
        - -T [name node identifier port]



#### Example Usage
Run program on fa23-cs425-7203 with version 0 with machine 1, with 0 drop rate, and machine 2 also in the system:
`./sdfs -v 0 -s fa23-cs425-7201.cs.illinois.edu -d 0 -o fa23-cs425-7202.cs.illinois.edu`


The program creates a folder called test where it places all its log files. The program takes input from stdin; the following inputs are supported:
1. **Grep Command:**
- `grep [args] [pattern]` 
To search for pattern 'hello' in the log files, provide `grep hello` as input to stdin, to search for all lines that do not contain 'hello', provide `grep -v hello` as input to stdin.
2. **Membership List:**
- `list_mem`
Prints membership list
- `list_self`
Prints information on current machine
- `enable suspicion`
Switches failure detection protocol of system to gossip with suspicion
- `disable suspicion`
Switches failure detection protocol of system to gossip (without suspicion)
3. **Join and Leave:**
- `join`
Contacts introducer and joins the system. Starts the gossip client and server, grep client and server, and if applicable, the introducer.
- `leave`
Gracefully leaves the system; the program remains alive but does not partake in gossiping. If program is running introducer, the introducer is killed.
4. **File Operations:**
- `put localfilename sdfsfilename` to upload a local file to the SDFS.
- `get sdfsfilename localfilename` to download a file from the SDFS to the local system.
- `delete sdfsfilename` to remove a file from the SDFS.
- `ls` to list all files in the system (namenode only).
- `store` to display files stored on the local node.



Note: we assume that the name of machines are `fa23-cs425-72[machine number].cs.illinois.edu`

## Compilation with tests
- Uncomment the desired test section in compile.sh and comment out the rest.
- Note: tests assume that the name of machines are `fa23-cs425-72[machine number].cs.illinois.edu`
