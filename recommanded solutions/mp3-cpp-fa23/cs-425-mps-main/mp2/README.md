# CS 425 - MP 2
Distributed Group Membership

## Description
Design for maintaining membership information using heartbeat style of failure detection. Uses Gossip with/without suspicion mechanism for failure drection.

## Design
#### Algorithm: 
In the distributed group membership system, every gossip period, each machine in the structure picks random b machines from membership and sends the random nodes its membership list, the protocol it uses, and a generation count of the protocol.

Memberhip list sent varies depending on failure detection protocol used. In Gossip, for each alive node in membership list, we sends its machine id, machine version, and latest heartbeat counter to other nodes. In gossip with suspicion, we also send the incarnation number and status of each node (i.e. suspect, failed, alive) along with the machine id, machine version, and latest heartbeat counter.

#### Join: 
The client sends a join request to the introducer server. New node sends its machine id and version as part of the request. Introducer adds the new node into its membership list and replies with its entire membership list. To facilitate the rejoining of the same machine, we assign distinct machine versions each a time a machine joins joins.

#### Regular operation: 
The client regularly updates its heartbeat and shares membership list with random b servers. Servers manages and updates the local membership list based on received data with UDP protocol. Since at most three machines can fail simultaneously, we let each machine communicate with at least four machines which are randomly picked.

#### Changing Gossip Protocol: 
If a machine is instructed to change protocol, the machine updates its protocol, increments the generation count, and disseminates this information to other servers in the gossip round.

#### Leave: 
The machine updates its own state to 'leave' and communicates this change to other nodes.

## Report
[report](https://docs.google.com/document/d/1_Kw6IGHsbuDKGIDhBqtq37NgsLPwKvHMjejMlRcm81w/edit?usp=sharing)

## Compilation
Compile program: 
`g++ -std=c++17 -g2 gossip_client.cpp gossip_server.cpp introducer.cpp main.cpp mp1_wrapper.cpp utils.cpp ../mp1/client.cpp ../mp1/server.cpp ../mp1/utils.cpp -I ../mp1 -lpthread -o membership_manager`

(equivalent to: `g++ -std=c++17 $(ls *.cpp | grep -v test) $(ls ../mp1/*.cpp | grep -v -E 'test|main|setup') -I ../mp1 -o membership_manager -lpthread`)

### Execution
Run program: `./membership_manager -n [id of the vm program is run on] -i [id of the introducer] -v [version number of the vm program is run on] 
		-o "[name of vm program is run on] [other vm name]..."`

All arguments provided to -o are used for MP 1 logic. When you run a grep command, the client will query machines provided after -o option.

Example: Run program on fa23-cs425-7203 with version 0 with machine 1 acting as the introducer:
`./membership_manager -n 3 -v 0 -i 1`

To use grep (from MP 1) with machines fa23-cs425-7203 and fa23-cs425-7202, supply argument `-o "fa23-cs425-7203.cs.illinois.edu fa23-cs425-7201.cs.illinois.edu fa23-cs425-7202.cs.illinois.edu"`

The program creates a folder called test where it places all its log files. The program takes input from stdin; the following inputs are supported:
1. `grep [args] [pattern]` 
To search for pattern 'hello' in the log files, provide `grep hello` as input to stdin, to search for all lines that do not contain 'hello', provide `grep -v hello` as input to stdin.
2. `list_mem`
Prints membership list
3. `list_self`
Prints information on current machine
4. `enable suspicion`
Switches failure detection protocol of system to gossip with suspicion
5. `disable suspicion`
Switches failure detection protocol of system to gossip (without suspicion)
6. `join`
Contacts introducer and joins the system. Starts the gossip client and server, grep client and server, and if applicable, the introducer.
7. `leave`
Gracefully leaves the system; the program remains alive but does not partake in gossiping. If program is running introducer, the introducer is killed.

Note: we assume that the name of machines are `fa23-cs425-72[machine number].cs.illinois.edu`

## Compilation with tests
To test membership list: `g++ -std=c++17 -g test_membership_list_manager.cpp utils.cpp ../mp1/utils.cpp -I ../mp1 -o membership_manager -lpthread && ./membership_manager`

To test entire system:
`g++ -std=c++17 -g test.cpp utils.cpp ../mp1/utils.cpp -I ../mp1 -o test_membership_manager -lpthread && ./test_membership_manager`

Note: tests assume that the name of machines are `fa23-cs425-72[machine number].cs.illinois.edu`
