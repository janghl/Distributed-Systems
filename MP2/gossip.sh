#!/bin/bash
USERNAME=hj33
HOSTS="fa23-cs425-6901.cs.illinois.edu fa23-cs425-6902.cs.illinois.edu fa23-cs425-6903.cs.illinois.edu fa23-cs425-6904.cs.illinois.edu fa23-cs425-6906.cs.illinois.edu fa23-cs425-6907.cs.illinois.edu fa23-cs425-6908.cs.illinois.edu fa23-cs425-6909.cs.illinois.edu fa23-cs425-6910.cs.illinois.edu"
SCRIPT="cd /home/hj33/distributed-log-querier/MP2; make gossip; ./gossip.out"
COUNTER=1
for HOSTNAME in ${HOSTS} ; do
    echo "sshing into {HOSTNAME} to start server"
    (ssh -l ${USERNAME} ${HOSTNAME} "${SCRIPT} ${COUNTER} &")&
    COUNTER=$((COUNTER+1))
done
