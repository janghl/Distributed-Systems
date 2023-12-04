# MP4
## Description

Based on the SDFS we built for MP3, we further implemented a mapreduce system. With files with predefined prefix in SDFS, the maple phase can process the files and generate a few temp files, each file per key. The juice phase then process all temp files and generate a final output.

We also deploy Hadoop across ten virtual machines. With one master and nine slaves, the cluster can process our maple and juice python files just like our implementation does.


## MP4: Steps to use (distributed file system)

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
* `maple maple.py number_of_parallel_jobs source_file_prefix intermidiate_prefix `: call maple.py to process all the files with fixed prefix and output intermidiate files per key
* `juice juice.py number_of_parallel_jobs intermidiate_prefix destination_file_prefix boolean_delete_file string_range_or_hash_partition`: call juice.py to process all the files with intermediate prefix and output results with optional choices of partition method 

### Example usage of Hadoop cluster

```
hadoop jar /home/hj33/hadoop-2.7.7/share/hadoop/tools/lib/hadoop-streaming-2.7.7.jar -mapper "filter_maple.py select all from Test.csv where Intersecti = '[A-Za-z]*L/[A-Za-z]*'" -file ~/filter_maple.py -reducer filter_juice.py -file ~/filter_juice.py  -file ~/Test.csv  -input /Test.csv -output /output   
```
see hadoop-command for details

### Example output of Hadoop

```
packageJobJar: [/home/hj33/demo_maple.py, /home/hj33/demo_juice.py, /home/hj33/Test.csv, /tmp/hadoop-unjar911859345952723905/] [] /tmp/streamjob7245307064885086677.jar tmpDir=null
23/12/03 08:44:27 INFO client.RMProxy: Connecting to ResourceManager at fa23-cs425-6901.cs.illinois.edu/172.22.94.228:8032
23/12/03 08:44:27 INFO client.RMProxy: Connecting to ResourceManager at fa23-cs425-6901.cs.illinois.edu/172.22.94.228:8032
23/12/03 08:44:28 INFO mapred.FileInputFormat: Total input paths to process : 1
23/12/03 08:44:28 INFO mapreduce.JobSubmitter: number of splits:2
23/12/03 08:44:28 INFO mapreduce.JobSubmitter: Submitting tokens for job: job_1701612488798_0002
23/12/03 08:44:29 INFO impl.YarnClientImpl: Submitted application application_1701612488798_0002
23/12/03 08:44:29 INFO mapreduce.Job: The url to track the job: http://fa23-cs425-6901.cs.illinois.edu:8088/proxy/application_1701612488798_0002/
23/12/03 08:44:29 INFO mapreduce.Job: Running job: job_1701612488798_0002
23/12/03 08:44:35 INFO mapreduce.Job: Job job_1701612488798_0002 running in uber mode : false
23/12/03 08:44:35 INFO mapreduce.Job:  map 0% reduce 0%
23/12/03 08:44:41 INFO mapreduce.Job:  map 100% reduce 0%
23/12/03 08:44:46 INFO mapreduce.Job:  map 100% reduce 100%
23/12/03 08:44:47 INFO mapreduce.Job: Job job_1701612488798_0002 completed successfully
23/12/03 08:44:47 INFO mapreduce.Job: Counters: 49
```

## Authors and acknowledgment
* [Joey Stack](mailto:jkstack2@illinois.edu)
* [Hanliang Jiang](mailto:hj33@illinois.edu)
