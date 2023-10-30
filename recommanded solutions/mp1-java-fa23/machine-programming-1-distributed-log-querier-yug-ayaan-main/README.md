# Machine Programming 1 – Distributed Log Querier - Yug + Ayaan

## Getting started

### Pulling the more recent changes:

Make sure to call `git pull origin main` before calling the main code

### How to run the main code:

On the command line, in the MP directory, make sure to run:
```
javac src/Client.java src/Server.java src/DistributedLogQuerier.java -d bin
cd bin
java MP1.DistributedLogQuerier 
```

This will compile the java code and run the main executable from the driver code in `DistributedLogQuerier.java`.

If you want to call the same commands with a bash script, make sure to call

```
./start.sh
```

### How to run unit tests

```
javac -d bin -cp /home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/junit-4.13.1.jar src/Server.java src/Client.java src/DistributedLogQuerier.java src/ServerTest.java src/ClientTest.java src/DistributedLogQuerierTest.java 
java -cp bin:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/hamcrest-2.2.jar:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/junit-4.13.1.jar org.junit.runner.JUnitCore MP1.DistributedLogQuerierTest # For the Distributed Log Querier Tests
java -cp bin:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/hamcrest-2.2.jar:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/junit-4.13.1.jar org.junit.runner.JUnitCore MP1.ServerTest # For the Server Tests
java -cp bin:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/hamcrest-2.2.jar:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/junit-4.13.1.jar org.junit.runner.JUnitCore MP1.ClientTest # For the Client Tests
```

If you want to call the same commands with a bash script, make sure to call

```
./test.sh
```
This will run all of them.

**NOTE: For Distributed Log Querier Tests, all of your virtual machines must be on. The machines (except the one running the unit tests if running from one of the VMs) should also be running the servers.**

## Authors and acknowledgment
* Yug Mittal (yugm2)
* Ayaan Shah (ayaanns2)
