package MP1;

import java.io.IOException;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Scanner;

/**
 * This class implements the Distributed Log Querier and triggers both the client and server on the machine
 *
 * Helpful resources: https://www.digitalocean.com/community/tutorials/java-socket-programming-server-client
 * https://www.w3schools.com/java/java_threads.asp
 */
public class DistributedLogQuerier {
    private static ArrayList<String> hostnames = new ArrayList<>();

    enum CommandState {
        GREP,
        EXIT,
        INVALID
    }

    /**
     * This checks the state of the command based on what is in the command output
     * 
     * @param command -- The command that we are trying to identify
     * @return enum for grep if command is grep (and has contents after "grep " (more than length of 5)). enum for exit if command is exit. Otherwise enum for invalid command
     * 
     */
    private static CommandState identifyCommandState(String command) {
        if (command.length() > 5 && command.indexOf("grep ") == 0) return CommandState.GREP;
        if (command.equalsIgnoreCase("exit")) return CommandState.EXIT;
        return CommandState.INVALID;
    }

    /**
     * This goes through all the servers, executes the grep command, and then collects and analyzes its results.
     * For the grep command, this gets the counts of the lines from each server and the total count
     * 
     * @param command - The grep command to send across all the servers (including this one)
     */
    private static void queryLogger(String command) {
        ArrayList<Client> clients = new ArrayList<Client>();
        SynchronizedPrinter printer = new SynchronizedPrinter();

        // Grep start time
        long start = System.nanoTime();

        // Go down the ip addresses list and spin up a thread for each
        for (int i = 0; i < hostnames.size(); i++) {
            Client client = new Client(command, hostnames.get(i), printer);
            client.start();
            clients.add(client);
        }

        // Wait for all threads to be done printing
        for (int i = 0; i < clients.size(); i++) {
            try {
                clients.get(i).join();
            } catch (Exception e) {
                printer.printErr(e);
            }
        }

        // Grep end time
        long stop = System.nanoTime();
        long duration = (stop - start) / 1000000;

        // Get lines printed for each thread
        int total_lines = 0;
        printer.printSummaryTitle(duration);
        for (int i = 0; i < clients.size(); i++) {
            clients.get(i).printSummary();
            total_lines += clients.get(i).getLines();
        }
        printer.printSummaryTotal(total_lines);
    }

    /**
     * This takes a server and then sends an exit command to kill it
     * 
     * @param server -- The server to kill
     */
    private static void killServer(Server server) {
        try {
            Client client = new Client("exit", InetAddress.getLocalHost().getHostName(), new SynchronizedPrinter());
            client.run();
            server.join();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Resets and adds the hosts to the ones to keep track of
     **/
    private static void addHosts() {
        // Make sure to reset hostnames when adding as hostnames is static (didn't reset during unit tests)
        hostnames = new ArrayList<>();
        hostnames.add("fa23-cs425-2201.cs.illinois.edu");
        hostnames.add("fa23-cs425-2202.cs.illinois.edu");
        hostnames.add("fa23-cs425-2203.cs.illinois.edu");
        hostnames.add("fa23-cs425-2204.cs.illinois.edu");
        hostnames.add("fa23-cs425-2205.cs.illinois.edu");
        hostnames.add("fa23-cs425-2206.cs.illinois.edu");
        hostnames.add("fa23-cs425-2207.cs.illinois.edu");
        hostnames.add("fa23-cs425-2208.cs.illinois.edu");
        hostnames.add("fa23-cs425-2209.cs.illinois.edu");
        hostnames.add("fa23-cs425-2210.cs.illinois.edu"); 
    }
    public static void main(String[] args) throws IOException, InterruptedException, ClassNotFoundException {
        System.out.print("Welcome to Yug and Ayaan's Distributed Grep. Type 'grep [query_text]' to begin.\nEnter Command: ");
        Scanner in = new Scanner(System.in);
        Server server = new Server();

        // Adds all hostnames
        addHosts();

        // Run server on a separate thread. All grep queries will go there.
        server.start();
        // Give server time to set up (especiallly in unit testing).
        server.sleep(1000);
        // Wait for a query 
        while (in.hasNextLine()) {
            String command = in.nextLine();
            CommandState commandState = identifyCommandState(command);
            if (commandState == CommandState.EXIT) break;
            if (commandState == CommandState.INVALID) {
                System.out.print("Invalid command, please try again\nCommand format: grep [query_text]\nType \"exit\" if you want to exit the program\nEnter Command: ");
                continue;
            }
            // Spawn client threads. Give them an id, mutex lock, and the command
            // This is blocking code in the current design. The queryLogger will wait until all client threads are done
            queryLogger(command);

            // Prints a new line to separate successive commands
            System.out.print("\nEnter Command: ");
        }

        // Cleanup
        System.out.println("Thanks for using our Distributed Grep. Cleaning up and closing.\nTo shut off the whole network type exit in each VM.");
        killServer(server);
        in.close();
    }
}
