# MP3
## Description

The MP implements a distributed file system(SDFS). To interact with the file system, we defined five operations: get file, put file, list local files, list availability of a file, delete file.

Each file in the SDFS has three replicas, so the system can tolerate up to three nodes crash. Furthermore, the leader list(i.e. a map like (file_name, replicas)) is maintained by all the nodes, so leader can be crash and reelected while maintaining a correct leader list.


## MP3: Steps to use (distributed file system)

Directly run `node.py`, and type in to see the content you want to know. For example, type in

* `list_mem`: print out the info of membership list, the membership list contains
  * host ID
  * heartbeat
  * time stamp
  * status
* `list_self`: print out host ID
* `join` and `leave`: After typing leave, the virtual machine will temporarily leave from the group(stop sending the message to other machines). Then type join to rejoin in the group(restart sending the message to other machines)
* `put local_filename sdfs_filename`: puts the "local_filename" in the local directory into the SDFS file system as "sdfs_filename"
* `get sdfs_filename local_filename`: gets the "sdfs_filename" in the SDFS and stores it locally as "local_filename"
* `delete sdfs_filename`: delete the "sdfs_filename" in the SDFS among all machines
* `ls sdfs_filename`: list the position for a specific "sdfs_filename"
* `store`: show all the file stored locally

### Example output

```
ID: fa23-cs425-8002.cs.illinois.edu:12345:1695523506, Heartbeat: 26, Status: Alive, Time: 1695523538.162461
ID: fa23-cs425-8004.cs.illinois.edu:12345:1695523523, Heartbeat: 63, Status: Alive, Time: 1695523539.1641357
ID: fa23-cs425-8005.cs.illinois.edu:12345:1695523524, Heartbeat: 59, Status: Alive, Time: 1695523539.164185
ID: fa23-cs425-8006.cs.illinois.edu:12345:1695523525, Heartbeat: 54, Status: Alive, Time: 1695523539.1641378
ID: fa23-cs425-8007.cs.illinois.edu:12345:1695523526, Heartbeat: 50, Status: Alive, Time: 1695523539.164137
ID: fa23-cs425-8008.cs.illinois.edu:12345:1695523527, Heartbeat: 46, Status: Alive, Time: 1695523539.1641614
ID: fa23-cs425-8009.cs.illinois.edu:12345:1695523529, Heartbeat: 40, Status: Alive, Time: 1695523539.1641872
ID: fa23-cs425-8010.cs.illinois.edu:12345:1695523533, Heartbeat: 24, Status: Alive, Time: 1695523539.1641393
```

## Authors and acknowledgment
* [Joey Stack](mailto:jkstack2@illinois.edu)
* [Hanliang Jiang](mailto:hj33@illinois.edu)
