# MP2
## Description

The MP is a failure detection system of heartbeating style using Gossip-style. The virtual machines can form a group and it can detect for any failure of any machine and the rejoin of the group.

There are two modes of the failure detection, that is Gossip mode and Gossip + S mode, which has a suspision mechanism.

***Parameters***

* `protocol-time`: The interval time for sending gossip message to other machines. With less protocol time, the machine will send message to other machines more frequently. The default time is 0.25s.
* `drop-rate`: The probability of dropping the receiving message to introduce the false positive rate

## Steps to Use

Directly run `node.py (-t protocol_time -d drop_rate)`, and type in to see the content you want to know. For example, type in

* `list_mem`: print out the info of membership list, the membership list contains
  * host ID
  * heartbeat
  * time stamp
  * status
* `list_self`: print out host ID
* `join` and `leave`: After typing leave, the virtual machine will temporarily leave from the group(stop sending the message to other machines). Then type join to rejoin in the group(restart sending the message to other machines)
* `enable_gossip` and `disable_gossip`: change to the GOSSIP+S mode and change back to GOSSIP mode

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
* [Chang Liu](mailto:changl25@illinois.edu)
* [An Phan](mailto:anphan2@illinois.edu)