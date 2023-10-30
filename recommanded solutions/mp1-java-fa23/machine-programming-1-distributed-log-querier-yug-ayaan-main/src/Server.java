package MP1;

import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;

/**
 * This class implements the Java Socket server that is to be run on
 * a separate thread. Hence why it extends the Thread class
 *
 * Helpful resources: https://www.digitalocean.com/community/tutorials/java-socket-programming-server-client
 * https://www.w3schools.com/java/java_threads.asp
 */
class Server extends Thread {
    private static boolean useTest = false;
    private ServerSocket serverSocket;
    private final int PORT = 8000;
    private static final int BUFFER_SIZE = 20 * 1048576;
    // Needs to be static for file to read from the path.
    private static final String LOG_FILE = "../machine.i.log";
    private static final String TEST_LOG_FILE = "../test.i.log";

    /**
     * When a new thread is created, this runs a server socket and listens to clients.
     * When listening to clients, it will see what to grep from machine.i.log, and
     * based on that, sends the grep info to the client
     */
    @Override
    public void run() {
        // Initialize server-socket
        try {
            serverSocket = new ServerSocket(PORT);
        } catch (Exception e) {
            System.err.println("Failed to create socket");
            e.printStackTrace();
        }

        while (true) {
            try {
                // Create server socket and accept to establish connection to clients
                Socket socket = serverSocket.accept();
                // BufferedReader and Writer
                BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()), BUFFER_SIZE);
                BufferedWriter out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()), BUFFER_SIZE);

                String message = in.readLine();

                //close resources and terminate the server if client sends exit request
                if (message.equalsIgnoreCase("exit")) {
                    in.close();
                    out.close();
                    socket.close();
                    break;
                }

                // FOR TESTING ONLY: This is for unit tests to trigger this to change the log files (the test one)
                if (message.indexOf("test") == 0) {
                    useTest = true;
                    reshuffleLog(in);
                    in.close();
                    out.close();
                    socket.close();
                    continue;
                }

                // FOR TESTING ONLY: This is for unit tests to trigger this to read machine.i.log instead
                if (message.indexOf("resetTest") == 0) {
                    useTest = false;
                    in.close();
                    out.close();
                    socket.close();
                    continue;
                }

                // To indicate to the server that we want to execute a grep command,
                // we include "grep " at the start (and the text after cannot be empty)
                String query = message.substring(5);
                //write grep output from machine.i.log to Socket
                executeGrep(out, query);
//                out.write(executeGrep(query));
                out.flush();
                //close resources
                in.close();
                out.close();
                socket.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        try {
            serverSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        System.err.println("Killing Server Thread");
    }

    /**
     * Used by unit tests. This changes the log file based on the input sent to the server
     *
     * @param in -- The input stream containing the contents to write the file into
     * @throws IOException
     */
    private static void reshuffleLog(BufferedReader in) throws IOException {
        BufferedWriter out = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(TEST_LOG_FILE)), BUFFER_SIZE);
        in.transferTo(out);
        out.close();
    }

    /**
     * Approach found in https://introcs.cs.princeton.edu/java/51language/Grep.java.html
     * Prints out lines containing the query from the file.
     *
     * @param query - The query to search for in the file
     * @param out - The stream to write the lines containing the keyword
     * @throws IOException if file does not exist
     */
    private static void executeGrep(BufferedWriter out, String query) throws IOException {
        // All of the String copying might end up being expensive.
        Runtime rt = Runtime.getRuntime();
        // Note: For unit testing, if triggered, the test log file will be used instead of the log file,
        // but it shouldn't logically make a difference on the grep behavior as a log file is being read
        // For unit testing, we would grep test.i.log similar to how we would read machine.i.log
        String[] commands = {"bash", "-c", "grep " + query + " " + (useTest ? TEST_LOG_FILE : LOG_FILE)};
        Process proc = rt.exec(commands);

        BufferedReader in = new BufferedReader(new
                InputStreamReader(proc.getInputStream()), BUFFER_SIZE);

        BufferedReader err = new BufferedReader(new
                InputStreamReader(proc.getErrorStream()), BUFFER_SIZE);

        // transferTo speeds up the process really well
        in.transferTo(out);
        in.close();
        err.transferTo(out);
        err.close();
    }
}
