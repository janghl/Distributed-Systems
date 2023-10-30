package MP1;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.ConnectException;
import java.net.InetSocketAddress;
import java.net.NoRouteToHostException;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketTimeoutException;

/**
 * Clients are synchronous threads, each of which communicates with a server on the network to acquire buffered data, printed in a thread-safe manner.
 */
class Client extends Thread {
    
    private String command;
    private String hostname;
    private SynchronizedPrinter printer;
    private final int PORT = 8000;
    private final int BUFFER_SIZE = 4096;
    private final int CONNECTION_TIMEOUT = 10000;
    private volatile int line_count = -1;
    
    public void printSummary() {
        printer.printSummaryLine(hostname, line_count);
    }

    public int getLines() {
        if (line_count > -1) return line_count;
        return 0;
    }
    
    /**
     * Constructor for Client
     * 
     * @param command - grep [query] OR exit
     * @param hostname - Name of host
     * @param printer - Instance of thread-safe printer shared among threads
     */
    Client(String command, String hostname, SynchronizedPrinter printer) {
        this.command = command;
        this.printer = printer;
        this.hostname = hostname;
    }

    /**
     * Connects to server, sends request to server, buffer-reads the response, and thread-safely prints it to stdout
     */
    @Override
    public void run() {
        try {
            Socket socket = new Socket();
            SocketAddress socketAddress = new InetSocketAddress(hostname, PORT);
            socket.connect(socketAddress, CONNECTION_TIMEOUT);
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()), BUFFER_SIZE);
            BufferedWriter out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));

            // send command
            out.write(command);
            out.flush();
            socket.shutdownOutput();
            
            // Wait for answer, synchronized print and count lines
            line_count = 0;
            String line;
            while ((line = in.readLine()) != null) {
                printer.printOut(line, hostname);
                line_count++;
            }

            // Clean up Assets
            in.close();
            out.close();
            socket.close();

        } catch (SocketTimeoutException | NoRouteToHostException e) {
            printer.printErr("Could not connect to host: " + hostname);
        } catch (ConnectException e) {
            printer.printErr("Connection refused for host: " + hostname);
        } catch (Exception e) {
            printer.printErr(e);
        }
    }
}

/**
 * Thread-Safe printer
 */
class SynchronizedPrinter {

    private final String ANSI_RED = "\u001B[31m";
    private final String ANSI_GREEN = "\u001B[32m";
    private final String ANSI_RESET = "\u001B[0m";	

    public void printOut (String line, String hostname) {
        String output = "[" + hostname + "] " + line;
        synchronized (System.out) {
            System.out.println(output);
        }
    }

    public void printErr (Exception e) {
        synchronized (System.err) {
            System.err.print(ANSI_RED);
            e.printStackTrace();
            System.err.print(ANSI_RESET);
        }
    }

    public void printErr (String line) {
        synchronized (System.err) {
            System.err.println(ANSI_RED + line + ANSI_RESET);
        }
    }

    public void printSummaryLine(String hostname, int line_count) {
        // This probably means there was an error
        if (line_count == -1) return;
        
        String line = ANSI_GREEN + hostname + " found " + line_count + " matching lines" + ANSI_RESET;
        synchronized(System.out) {
            System.out.println(line);
        }
    }

    public void printSummaryTitle(long duration) {
        synchronized(System.out) {
            System.out.println(ANSI_GREEN + "\nDistributedLogQuerier Summary:\nTime elapsed = " + duration + "ms" + ANSI_RESET);
        }
    }

    public void printSummaryTotal(int total_lines) {
        synchronized(System.out) {
            System.out.println(ANSI_GREEN + "Total matching lines = " + total_lines + "\n" + ANSI_RESET);
        }
    }
}
