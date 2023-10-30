javac -d bin -cp /home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/junit-4.13.1.jar src/Server.java src/Client.java src/DistributedLogQuerier.java src/ServerTest.java src/ClientTest.java src/DistributedLogQuerierTest.java 
echo "Running Distributed Log Querier Tests now"
java -cp bin:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/hamcrest-2.2.jar:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/junit-4.13.1.jar org.junit.runner.JUnitCore MP1.DistributedLogQuerierTest # For the Distributed Log Querier Tests
echo "Running Server tests now"
java -cp bin:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/hamcrest-2.2.jar:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/junit-4.13.1.jar org.junit.runner.JUnitCore MP1.ServerTest # For the Server Tests
echo "Running Client Tests now"
java -cp bin:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/hamcrest-2.2.jar:/home/yugm2/machine-programming-1-distributed-log-querier-yug-ayaan/junit-4.13.1.jar org.junit.runner.JUnitCore MP1.ClientTest # For the Client Tests