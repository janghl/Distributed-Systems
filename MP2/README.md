# Distributed Log Querier

This program implements a failure detection with both push gossip and gossip with suspicion mechanism.

## Details

 - This program creates background threads for four functionalities
 - The sender thread sends membership list randomly with push gossip
 - The receiver thread receives membership lists periodically and merge them with local list
 - The checker thread checks availablity of each entries and update status of each entries
 - The monitor thread deals with user command line interface

## Usage

# gossip failure detection
To maintain a distributed failure detection network:
```bash
make gossip
./server.out &
```
# unit_test
To test the functionalities of different modules:
```bash
make unit_test
```
# gossip
To manually start service among servers
```bash
bash gossip.sh
```

# gossip debug
To debug for the gossip code
```bash
make gossip_debug
```
