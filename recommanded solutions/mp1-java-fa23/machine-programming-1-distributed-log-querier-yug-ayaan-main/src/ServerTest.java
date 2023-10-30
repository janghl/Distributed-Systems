package MP1;
import org.junit.Before;
import org.junit.Test;

import java.io.*;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import static org.junit.Assert.*;
import static java.lang.Thread.sleep;

public class ServerTest {

    private final String HOSTNAME = InetAddress.getLocalHost().getHostName();
    private final int PORT = 8000;
    private final int BUFFER_SIZE = 4096;
    private final int SLEEP_TIME = 1500;
    private Socket socket;
    private Server server;
    private static final String LOG_FILE = "../test.i.log";

    /**
     * Default server test class constructor. Initializes server with
     *
     * @throws UnknownHostException -- Throws this if there is no local host for this server.
     */
    public ServerTest() throws UnknownHostException {
    }

    /**
     * This tells the server to rewrite the contents of the log file. Used for
     * testing purposes only.
     *
     * @param logfileData -- The contents to fill the log file with
     * @throws IOException -- If file not found
     * @throws InterruptedException -- If server thread is interrupted
     */
    private void setupLogfile(String logfileData) throws IOException, InterruptedException {
        BufferedWriter out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));

        out.write("test\n" + logfileData);
        out.flush();
        socket.shutdownOutput();
        out.close();
        socket.close();
        socket = new Socket(HOSTNAME, PORT);
    }

    @Before
    public void setUp() throws IOException, InterruptedException {
        server = new Server();
        server.start();
        // Give server time to set up
        sleep(SLEEP_TIME);
        socket = new Socket(HOSTNAME, PORT);
    }

    @Test
    public void exitTest() throws IOException, InterruptedException {
        BufferedWriter out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));

        // send command
        out.write("exit\n");
        out.flush();
        socket.shutdownOutput();
        // Give server thread time to finish executing
        sleep(SLEEP_TIME);
        // Server should not be alive after exit command is sent
        assertFalse(server.isAlive());
        server.join();
        out.close();
        socket.close();
    }

    @Test
    public void testLogChange() throws IOException, InterruptedException {
        setupLogfile("Yug goes to County Market\nAyaan writes some code\nYug and Ayaan are working on this MP\n");
        sleep(SLEEP_TIME);
        // Server should still be alive after
        assertEquals(true, server.isAlive());

        BufferedWriter out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
        out.write("exit\n");
        out.flush();
        socket.shutdownOutput();
        sleep(SLEEP_TIME);
        assertEquals(false, server.isAlive());
        server.join();

        BufferedReader inF = new BufferedReader(new FileReader(LOG_FILE));
        assertEquals("Yug goes to County Market", inF.readLine());
        assertEquals("Ayaan writes some code", inF.readLine());
        assertEquals("Yug and Ayaan are working on this MP", inF.readLine());
        // Those three lines should be the first in the file. Nothing to read after that
        assertEquals(inF.read(), -1);
        inF.close();
        out.close();
    }

    @Test
    public void testResetTest() throws IOException, InterruptedException {
        setupLogfile("Yug goes to County Market\nAyaan writes some code\nYug and Ayaan are working on this MP\n");
        sleep(SLEEP_TIME);
        // Server should still be alive after
        assertTrue(server.isAlive());

        BufferedWriter out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
        // send command
        out.write("resetTest\n");
        out.flush();
        socket.shutdownOutput();
        // Give server thread time to finish executing
        sleep(SLEEP_TIME);
        // Server should still be alive after resetCommand command is sent
        assertTrue(server.isAlive());
        out.close();
        socket.close();

        // None of the machine.i.log files should have "Yug" if running with the machines.
        socket = new Socket(HOSTNAME, PORT);
        out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
        BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()), BUFFER_SIZE);
        out.write("grep Yug\n");
        out.flush();
        StringBuilder finalString = new StringBuilder();
        char[] buffer = new char[BUFFER_SIZE];
        while (in.read(buffer) != -1) {
            finalString.append(buffer);
        }
        socket.shutdownOutput();
        sleep(SLEEP_TIME);
        // Server should still be alive after grep
        assertEquals(true, server.isAlive());
        in.close();
        out.close();
        socket.close();

        socket = new Socket(HOSTNAME, PORT);
        out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
        out.write("exit\n");
        out.flush();
        socket.shutdownOutput();
        sleep(SLEEP_TIME);
        assertFalse(server.isAlive());
        server.join();
        socket.close();
        out.close();

        assertFalse(finalString.toString().contains("Yug goes to County Market"));
        assertFalse(finalString.toString().contains("Ayaan writes some code"));
        assertFalse(finalString.toString().contains("Yug and Ayaan are working on this MP"));
    }

    @Test
    public void testGrepLog() throws IOException, InterruptedException {
        setupLogfile("Yug goes to the market\nAyaan writes some code\nYug and Ayaan are working on this MP\n");

        BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()), BUFFER_SIZE);
        BufferedWriter out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));

        // send query 1: "Yug"
        out.write("grep Yug\n");
        out.flush();
        StringBuilder finalString = new StringBuilder();
        char[] buffer = new char[BUFFER_SIZE];
        while (in.read(buffer) != -1) {
            finalString.append(buffer);
        }
        socket.shutdownOutput();
        assertTrue(finalString.toString().contains("Yug goes to the market"));
        assertFalse(finalString.toString().contains("Ayaan writes some code"));
        assertTrue(finalString.toString().contains("Yug and Ayaan are working on this MP"));
        sleep(SLEEP_TIME);
        // Server should still be alive after grep
        assertEquals(true, server.isAlive());
        socket.close();
        in.close();
        out.close();

        // Create new socket connection to server
        socket = new Socket(HOSTNAME, PORT);
        // Set up read and write streams
        in = new BufferedReader(new InputStreamReader(socket.getInputStream()), BUFFER_SIZE);
        out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
        // send query 2: "a"
        out.write("grep a\n");
        out.flush();
        finalString = new StringBuilder();
        buffer = new char[BUFFER_SIZE];
        while (in.read(buffer) != -1) {
            finalString.append(buffer);
        }
        // grep of "a" should contain all of the entries as all of them contain "a"
        assertTrue(finalString.toString().contains("Yug goes to the market"));
        assertTrue(finalString.toString().contains("Ayaan writes some code"));
        assertTrue(finalString.toString().contains("Yug and Ayaan are working on this MP"));
        sleep(SLEEP_TIME);
        // Server should still be alive after grep command
        assertTrue(server.isAlive());
        socket.close();
        in.close();
        out.close();

        // Socket connection for next grep input
        socket = new Socket(HOSTNAME, PORT);
        in = new BufferedReader(new InputStreamReader(socket.getInputStream()), BUFFER_SIZE);
        out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
        // grep test for "Ayaan"
        out.write("grep Ayaan\n");
        out.flush();
        finalString = new StringBuilder();
        buffer = new char[BUFFER_SIZE];
        while (in.read(buffer) != -1) {
            finalString.append(buffer);
        }
        // output should only contain strings only containing "Ayaan"
        assertFalse(finalString.toString().contains("Yug goes to the market"));
        assertTrue(finalString.toString().contains("Ayaan writes some code"));
        assertTrue(finalString.toString().contains("Yug and Ayaan are working on this MP"));
        sleep(SLEEP_TIME);
        assertTrue(server.isAlive());

        socket = new Socket(HOSTNAME, PORT);
        out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
        out.write("exit\n");
        out.flush();
        socket.shutdownOutput();
        sleep(SLEEP_TIME);
        assertFalse(server.isAlive());
        server.join();
        socket.close();
        in.close();
        out.close();
    }


}