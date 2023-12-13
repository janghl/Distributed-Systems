# !/bin/bash
# for each slave:
# usage: . deployment.sh
# the master gives ssh rsa key to slaves
rm -rf ~/h*
cd ~
wget https://archive.apache.org/dist/hadoop/core/hadoop-2.7.7/hadoop-2.7.7.tar.gz
tar -zxf hadoop-2.7.7.tar.gz
rm hadoop-2.7.7.tar.gz
rm ~/.ssh/* -rf
mv /home/hj33/distributed-log-querier/MP4/hadoop-modified/.bashrc /home/hj33/
mv /home/hj33/distributed-log-querier/MP4/hadoop-modified/* /home/hj33/hadoop-2.7.7/etc/hadoop
source .bashrc
