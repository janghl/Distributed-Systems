package MP1;
import org.junit.Before;
import org.junit.Test;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import static org.junit.Assert.*;
import static java.lang.Thread.sleep;

public class ClientTest {

    private final String HOSTNAME = InetAddress.getLocalHost().getHostName();
    private Server server;

    /**
     * Create the Client Test class
     * @throws UnknownHostException -- Throws this if there is no local host.
     */
    public ClientTest() throws UnknownHostException {
    }

    @Before
    public void setUp() throws IOException, UnknownHostException, InterruptedException {
        server = new Server();
        server.start();
        sleep(1000);
    }

    /**
     * This checks for when client tells server to exit
     * 
     * @throws IOException -- If cannot close file streams (i.e OutputStream)
     * @throws InterruptedException -- If thread is being interrupted (i.e Thread.sleep)
     */
    @Test
    public void clientExitsServer() throws IOException, InterruptedException {
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setErr(new PrintStream(myErr));
        Client client = new Client("exit\n", HOSTNAME, new SynchronizedPrinter());
        client.start();
        // Wait for thread to finish executing
        sleep(1000);
        // Client thread should be finished
        assertFalse(client.isAlive());
        // Error stream should recieve that server thread is dead
        assertTrue(myErr.toString().contains("Killing Server Thread"));

        // Remember to close everything before finishing up
        myErr.close();
        client.join();
        server.join();
    }

    /**
     * 
     * @throws IOException
     * @throws InterruptedException
     */
    @Test
    public void clientWithGrep() throws IOException, InterruptedException {
        // Set system output, so then we can read from the system output
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        // Update log file with what we want to test with
        Client logUpdate = new Client("test\nYug goes to County Market\nAyaan goes to Target\n",  HOSTNAME, new SynchronizedPrinter());
        logUpdate.start();
        sleep(1000);
        logUpdate.join();

        Client client = new Client("grep Yug", HOSTNAME, new SynchronizedPrinter());
        client.start();
        // Wait for thread to finish executing
        sleep(1000);
        // Client thread should not be alive after command is fully executed
        assertFalse(client.isAlive());
        client.join();
        // Grep command should print out "Yug goes to County Market"
        assertTrue(myOut.toString().contains("Yug goes to County Market"));
        assertFalse(myOut.toString().contains("Ayaan goes to Target"));
        myOut.close();

        myOut = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        client = new Client("grep Ayaan", HOSTNAME, new SynchronizedPrinter());
        client.start();
        // Wait for thread to finish executing
        sleep(1000);
        // Client thread should not be alive after command is fully executed
        assertFalse(client.isAlive());
        client.join();
        // Grep command should print out "Ayaan goes to Target"
        assertFalse(myOut.toString().contains("Yug goes to County Market"));
        assertTrue(myOut.toString().contains("Ayaan goes to Target"));
        myOut.close();

        // Now exit the server (the Client way)
        client = new Client("exit\n", HOSTNAME, new SynchronizedPrinter());
        client.start();
        client.join();
    }

    @Test
    public void multipleClientsWithGrep() throws IOException, InterruptedException {
        // Set system output, so then we can read from the system output
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        // Update log file with what we want to test with
        Client logUpdate = new Client("test\nYug goes to County Market\nAyaan goes to Target\n",  HOSTNAME, new SynchronizedPrinter());
        logUpdate.start();
        sleep(1000);
        logUpdate.join();

        Client client = new Client("grep Yug", HOSTNAME, new SynchronizedPrinter());
        Client client2 = new Client("grep Ayaan", HOSTNAME, new SynchronizedPrinter());
        client.start();
        client2.start();
        // Wait for thread to finish executing
        sleep(1000);
        sleep(1000);
        // Client thread should not be alive after command is fully executed
        assertFalse(client.isAlive());
        assertFalse(client2.isAlive());
        client.join();
        client2.join();
        // Grep command should print out "Yug goes to County Market"
        assertTrue(myOut.toString().contains("Yug goes to County Market"));
        assertTrue(myOut.toString().contains("Ayaan goes to Target"));
        myOut.close();

        // Now exit the server (the Client way)
        client = new Client("exit\n", HOSTNAME, new SynchronizedPrinter());
        client.start();
        client.join();
    }
}