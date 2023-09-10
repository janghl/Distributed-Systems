#!/bin/bash
USERNAME=js66
HOSTS="fa23-cs425-6901.cs.illinois.edu fa23-cs425-6902.cs.illinois.edu fa23-cs425-6903.cs.illinois.edu fa23-cs425-6904.cs.illinois.edu fa23-cs425-6906.cs.illinois.edu fa23-cs425-6907.cs.illinois.edu fa23-cs425-6908.cs.illinois.edu fa23-cs425-6909.cs.illinois.edu fa23-cs425-6910.cs.illinois.edu"
SCRIPT="cd /home/js66/distributed-log-querier; pkill server"
for HOSTNAME in ${HOSTS} ; do
    echo "sshing into ${HOSTNAME} to kill server"
    (ssh -l ${USERNAME} ${HOSTNAME} "${SCRIPT}")&
done
