package MP1;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.*;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;

import static org.junit.Assert.*;
import static java.lang.Thread.sleep;

/**
 * Unit tests for the Distributed Log Querier
 */
public class DistributedLogQuerierTest {
    private final String APP_OPEN_MESSAGE = "Welcome to Yug and Ayaan's Distributed Grep. Type 'grep [query_text]' to begin.\nEnter Command: ";

    private final String INVALID_COMMAND_MESSAGE = "Invalid command, please try again\nCommand format: grep [query_text]\nType \"exit\" if you want to exit the program\nEnter Command: ";

    private static ArrayList<String> hostnames = new ArrayList<>();

    private ArrayList<String> knownWords = new ArrayList<>();

    private static final String TEST_LOG_FILE = "../test.i.log";

    /**
     * This add the host names of the VMs that will run the server.
     */
    private void addHosts() {
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

    /**
     * This is for keeping track on Known words we would supply to the log files
     */
    private void addKnownWords() {
        knownWords = new ArrayList<>();
        knownWords.add("Yug goes to the market\nAyaan changes some code\nCS 425 Rocks\nYug wrote these unit tests");
        knownWords.add("Yug goes to the market\nWe need to test this comprehensively\nDELETE server");
        knownWords.add("DELETE server\nAyaan is experimenting\nThis will change the log file to this");
        knownWords.add("Yug wrote the unit test");
        knownWords.add("Yug goes to County Market\nAyaan goes to Target\nCS 425 4 credit hour section Rocks\nYug wrote these unit tests");
        knownWords.add("Yug goes to County Market\nAyaan goes to Target\nCS 425 4 credit hour section Rocks");
        knownWords.add("GET /list HTTP/1.0\nPOST /apps/cart.jsp?appID=6479\nGET /posts/posts/explore HTTP/1.0");
        knownWords.add("Yug wrote the unit tests");
        knownWords.add("Watch out on Piazza\nAyaan did the hard work");
        knownWords.add("Yug and Ayaan are finalized");
    }

    /**
     * This is for checking if client is able to connect to the servers at the other machines.
     * Note VMs have to be on and run the servers (except the one running the tests) for the grep tests to execute 
     * 
     * @param myErr -- The error stream to check if able to connect to other servers
     * @throws UnknownHostException -- 
     */
    private void assertHostsOn(ByteArrayOutputStream myErr) throws UnknownHostException {
        String localhost = InetAddress.getLocalHost().getHostName();
        for (String hostname : hostnames) {
            if (!hostname.equals(localhost)) {
                // Only VMs that are not running the unit tests should run the servers.
                assertFalse(myErr.toString().contains("Connection refused for host: " + hostname));
                assertFalse(myErr.toString().contains("Could not connect to host: " + hostname));
            }
        }
    }

    /**
     * This updates the log files across the servers to different texts for testing.
     * @throws IOException -- If cannot open file
     * @throws UnknownHostException -- If cannot find the local host
     */
    private void updateLogFile() throws IOException, UnknownHostException {
        String localhost = InetAddress.getLocalHost().getHostName();
        for (int i = 0; i < hostnames.size(); i++) {
            if (!hostnames.get(i).equals(localhost)) {
                Client client = new Client("test \n" + knownWords.get(i), hostnames.get(i), new SynchronizedPrinter());
                client.start();
            }
            else {
                BufferedWriter out = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(TEST_LOG_FILE)), 4096);
                out.write(knownWords.get(i));
                out.close();
            }
        }
    }

    @Before
    public void setUp() throws IOException {
        addHosts();
        addKnownWords();
    }

    /**
     * Tests for what would happen if the user just opened the application
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testOpenApplication() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stdout = System.out;
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        ByteArrayInputStream in = new ByteArrayInputStream("sdsd".getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);
        sleep(1500);
        assertTrue(myOut.toString().contains(APP_OPEN_MESSAGE));
        assertTrue(myOut.toString().contains(APP_OPEN_MESSAGE));
        System.setOut(stdout);
        myOut.close();
    }

    /**
     * Tests for what would happen if the user typed "exit"
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testExit() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream("exit\n".getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);
        sleep(1500);
        assertTrue(myErr.toString().contains("Killing Server Thread"));
        myErr.close();
        System.setErr(stderr);
    }

    /**
     * This tests for how the program responds if a user has put in an invalid command
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testInvalidCommand() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stdout = System.out;
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        ByteArrayInputStream in = new ByteArrayInputStream("nasnsn\n".getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);
        assertTrue(myOut.toString().contains(INVALID_COMMAND_MESSAGE));
        myOut.close();

        System.setOut(stdout);
    }

    /*
    Important: For this Unit Test, ensure all the VMs are on and servers are running on them
     */
    /**
     * Tests for Distributed grep across all the servers
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testOverallGrep() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        PrintStream stdout = System.out;

        updateLogFile();
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream(("grep Yug\n").getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);

        // Verify if all VMs are on and can connect to their server.
        // (the one invoking this test doesn't need to run the server as the unit tests will do it automatically).
        // This test will fail if the VMs are off or are not running the servers
        assertHostsOn(myErr);

        // All lines containing "Yug" should be in the output
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug wrote these unit tests"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2202.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2204.cs.illinois.edu] Yug wrote the unit test"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug wrote these unit tests"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2208.cs.illinois.edu] Yug wrote the unit tests"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2210.cs.illinois.edu] Yug and Ayaan are finalized"));

        // Any line not containing "Yug" should not be in the output
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Ayaan changes some code"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] CS 425 Rocks"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] Ayaan is experimenting"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Ayaan goes to Target"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2207.cs.illinois.edu] POST /apps/cart.jsp?appID=6479"));

        assertTrue(myOut.toString().contains("fa23-cs425-2201.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2202.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2204.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2207.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2208.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2210.cs.illinois.edu found 1 matching lines"));

        assertTrue(myOut.toString().contains("Total matching lines = 9"));

        myOut.close();
        myErr.close();

        System.setOut(stdout);
        System.setErr(stderr);
    }

    /**
     * Tests for Distributed grep across all the servers (Test two: Another letter)
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testOverallGrepTwo() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        PrintStream stdout = System.out;

        updateLogFile();
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream(("grep Ayaan\n").getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);

        // Verify if all VMs are on and can connect to their server.
        // (the one invoking this test doesn't need to run the server as the unit tests will do it automatically).
        // This test will fail if the VMs are off or are not running the servers
        assertHostsOn(myErr);

        // Any line not containing "Ayaan" should not be in the output
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug goes to the market"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] CS 425 Rocks"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug wrote these unit tests"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2202.cs.illinois.edu] Yug goes to the market"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2204.cs.illinois.edu] Yug wrote the unit test"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug goes to County Market"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug wrote these unit tests"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Yug goes to County Market"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2207.cs.illinois.edu] POST /apps/cart.jsp?appID=6479"));

        // All lines containing "Ayaan" should be in the output
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Ayaan changes some code"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] Ayaan is experimenting"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Ayaan did the hard work"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2210.cs.illinois.edu] Yug and Ayaan are finalized"));

        // The counts of the lines
        assertTrue(myOut.toString().contains("fa23-cs425-2201.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2202.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2204.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2207.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2208.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2210.cs.illinois.edu found 1 matching lines"));

        // Total number of matching lines should be sum of number of matching lines of each server
        assertTrue(myOut.toString().contains("Total matching lines = 6"));

        System.setOut(stdout);
        System.setErr(stderr);

        myOut.close();
        myErr.close();
    }

    /**
     * Tests for Distributed grep across all the servers (Test three: Regular expression (single letter))
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testOverallGrepThree() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        PrintStream stdout = System.out;

        updateLogFile();
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream(("grep a\n").getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);

        // Verify if all VMs are on and can connect to their server.
        // (the one invoking this test doesn't need to run the server as the unit tests will do it automatically).
        // This test will fail if the VMs are off or are not running the servers
        assertHostsOn(myErr);

        // Any line not containing "a" should not be in the output
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] CS 425 Rocks"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug wrote these unit tests"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2204.cs.illinois.edu] Yug wrote the unit test"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug wrote these unit tests"));

        // All lines containing "a" should be in the output
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Ayaan changes some code"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2202.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] Ayaan is experimenting"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] This will change the log file to this"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2207.cs.illinois.edu] POST /apps/cart.jsp?appID=6479"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Watch out on Piazza"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Ayaan did the hard work"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2210.cs.illinois.edu] Yug and Ayaan are finalized"));


        assertTrue(myOut.toString().contains("fa23-cs425-2201.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2202.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2204.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2207.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2208.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2210.cs.illinois.edu found 1 matching lines"));

        assertTrue(myOut.toString().contains("Total matching lines = 13"));

        myOut.close();
        myErr.close();
    }

    /**
     * Tests for Distributed grep across all the servers (Test four: grep of sentence with space (must have quotes))
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testOverallGrepFour() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        PrintStream stdout = System.out;

        updateLogFile();
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream(("grep \"Yug goes\"\n").getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);

        // Verify if all VMs are on and can connect to their server.
        // (the one invoking this test doesn't need to run the server as the unit tests will do it automatically).
        // This test will fail if the VMs are off or are not running the servers
        assertHostsOn(myErr);

        // Any line not containing "Yug goes" should not be in the output
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] CS 425 Rocks"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug wrote these unit tests"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2204.cs.illinois.edu] Yug wrote the unit test"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug wrote these unit tests"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Ayaan changes some code"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] Ayaan is experimenting"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] This will change the log file to this"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Ayaan goes to Target"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Ayaan goes to Target"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2207.cs.illinois.edu] POST /apps/cart.jsp?appID=6479"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Watch out on Piazza"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Ayaan did the hard work"));
        assertFalse(myOut.toString().contains("[fa23-cs425-2210.cs.illinois.edu] Yug and Ayaan are finalized"));


        // All lines containing "Yug goes" should be in the output
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2202.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Yug goes to County Market"));


        assertTrue(myOut.toString().contains("fa23-cs425-2201.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2202.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2204.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2207.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2208.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2210.cs.illinois.edu found 0 matching lines"));

        assertTrue(myOut.toString().contains("Total matching lines = 4"));

        myOut.close();
        myErr.close();
    }

    /**
     * Tests for Multiple greps. Should output both 
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testMultipleGrep() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        PrintStream stdout = System.out;

        updateLogFile();
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream(("grep \"Yug goes\"\ngrep a\n").getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);

        // Verify if all VMs are on and can connect to their server.
        // (the one invoking this test doesn't need to run the server as the unit tests will do it automatically).
        // This test will fail if the VMs are off or are not running the servers
        assertHostsOn(myErr);


        // All lines containing "Yug goes" should be in the output
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2202.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Yug goes to County Market"));


        assertTrue(myOut.toString().contains("fa23-cs425-2201.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2202.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2204.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2207.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2208.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2210.cs.illinois.edu found 0 matching lines"));
        
        assertTrue(myOut.toString().contains("Total matching lines = 4"));

        // All lines containing "a" should be in the output
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Ayaan changes some code"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2202.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] Ayaan is experimenting"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] This will change the log file to this"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2207.cs.illinois.edu] POST /apps/cart.jsp?appID=6479"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Watch out on Piazza"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Ayaan did the hard work"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2210.cs.illinois.edu] Yug and Ayaan are finalized"));

        // Matching lines for output containing "a"
        assertTrue(myOut.toString().contains("fa23-cs425-2201.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2202.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2204.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2207.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2208.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2210.cs.illinois.edu found 1 matching lines"));

        assertTrue(myOut.toString().contains("Total matching lines = 13"));

        myOut.close();
        myErr.close();
    }

    /**
     * Tests for grep then Invalid command
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testGrepTheInvalidCommand() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        PrintStream stdout = System.out;

        updateLogFile();
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream(("grep a\najshgah\n").getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);

        // Verify if all VMs are on and can connect to their server.
        // (the one invoking this test doesn't need to run the server as the unit tests will do it automatically).
        // This test will fail if the VMs are off or are not running the servers
        assertHostsOn(myErr);

        // Invalid command should output this
        assertTrue(myOut.toString().contains(INVALID_COMMAND_MESSAGE));

        // All lines containing "a" should be in the output
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Ayaan changes some code"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2202.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] Ayaan is experimenting"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] This will change the log file to this"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2207.cs.illinois.edu] POST /apps/cart.jsp?appID=6479"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Watch out on Piazza"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Ayaan did the hard work"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2210.cs.illinois.edu] Yug and Ayaan are finalized"));

        // Invalid command should come after grep output
        assertTrue(myOut.toString().indexOf(INVALID_COMMAND_MESSAGE) > myOut.toString().indexOf("[fa23-cs425-2201.cs.illinois.edu]"));

        // Matching lines for output containing "a"
        assertTrue(myOut.toString().contains("fa23-cs425-2201.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2202.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2204.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2207.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2208.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2210.cs.illinois.edu found 1 matching lines"));

        assertTrue(myOut.toString().contains("Total matching lines = 13"));

        myOut.close();
        myErr.close();
    }

    /**
     * Tests for Invalid command then grep. Should invoke 
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testInvalidCommandThenGrep() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        PrintStream stdout = System.out;

        updateLogFile();
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream(("ajshgah\ngrep a").getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);

        // Verify if all VMs are on and can connect to their server.
        // (the one invoking this test doesn't need to run the server as the unit tests will do it automatically).
        // This test will fail if the VMs are off or are not running the servers
        assertHostsOn(myErr);

        // Invalid command should output this
        assertTrue(myOut.toString().contains(INVALID_COMMAND_MESSAGE));

        // All lines containing "a" should be in the output
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2201.cs.illinois.edu] Ayaan changes some code"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2202.cs.illinois.edu] Yug goes to the market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] Ayaan is experimenting"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2203.cs.illinois.edu] This will change the log file to this"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2205.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Yug goes to County Market"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2206.cs.illinois.edu] Ayaan goes to Target"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2207.cs.illinois.edu] POST /apps/cart.jsp?appID=6479"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Watch out on Piazza"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2209.cs.illinois.edu] Ayaan did the hard work"));
        assertTrue(myOut.toString().contains("[fa23-cs425-2210.cs.illinois.edu] Yug and Ayaan are finalized"));

        // Invalid command should come before grep output
        assertTrue(myOut.toString().indexOf(INVALID_COMMAND_MESSAGE) < myOut.toString().indexOf("[fa23-cs425-2201.cs.illinois.edu]"));

        // Matching lines for output containing "a"
        assertTrue(myOut.toString().contains("fa23-cs425-2201.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2202.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2204.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2207.cs.illinois.edu found 1 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2208.cs.illinois.edu found 0 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu found 2 matching lines"));
        assertTrue(myOut.toString().contains("fa23-cs425-2210.cs.illinois.edu found 1 matching lines"));

        assertTrue(myOut.toString().contains("Total matching lines = 13"));

        myOut.close();
        myErr.close();
    }

    /**
     * Tests for Exit then grep (should not be possible, so we test for it)
     * One thing to note, since the driver keeps reading while there is a new line, as we supply new lines to the input,
     * we keep reading. So this tests what happens if we exit before we do anything else
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testExitThenGrep() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        PrintStream stdout = System.out;

        updateLogFile();
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream(("exit\ngrep a\n").getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);

        // Verify if all VMs are on and can connect to their server.
        // (the one invoking this test doesn't need to run the server as the unit tests will do it automatically).
        // This test will fail if the VMs are off or are not running the servers
        assertHostsOn(myErr);

        // Nothing should be outputted by grep if exited already
        assertFalse(myOut.toString().contains("fa23-cs425-2201.cs.illinois.edu]"));
        assertFalse(myOut.toString().contains("fa23-cs425-2202.cs.illinois.edu]"));
        assertFalse(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu"));
        assertFalse(myOut.toString().contains("fa23-cs425-2203.cs.illinois.edu"));
        assertFalse(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu"));
        assertFalse(myOut.toString().contains("fa23-cs425-2205.cs.illinois.edu"));
        assertFalse(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu"));
        assertFalse(myOut.toString().contains("fa23-cs425-2206.cs.illinois.edu"));
        assertFalse(myOut.toString().contains("fa23-cs425-2207.cs.illinois.edu"));
        assertFalse(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu"));
        assertFalse(myOut.toString().contains("fa23-cs425-2209.cs.illinois.edu"));
        assertFalse(myOut.toString().contains("fa23-cs425-2210.cs.illinois.edu"));

        // Nothing should be found if exited early
        assertFalse(myOut.toString().contains("found"));

        assertFalse(myOut.toString().contains("Total matching lines = 13"));

        myOut.close();
        myErr.close();
    }

    /**
     * Tests for Exit then grep (should not be possible, so we test for it)
     * One thing to note, since the driver keeps reading while there is a new line, as we supply new lines to the input,
     * we keep reading. So this tests what happens if we exit before we do anything else
     *
     * @throws IOException - If files are not found
     * @throws InterruptedException - If a thread is interrupted (i.e Thread.sleep)
     * @throws ClassNotFoundException - To check if DistributedLoqQuerier is found when calling main 
     */
    @Test
    public void testExitThenInvalid() throws IOException, InterruptedException, ClassNotFoundException {
        PrintStream stderr = System.err;
        PrintStream stdout = System.out;

        updateLogFile();
        ByteArrayOutputStream myOut = new ByteArrayOutputStream();
        ByteArrayOutputStream myErr = new ByteArrayOutputStream();
        System.setOut(new PrintStream(myOut));
        System.setErr(new PrintStream(myErr));
        ByteArrayInputStream in = new ByteArrayInputStream(("exit\ngsdsd").getBytes());
        System.setIn(in);
        DistributedLogQuerier.main(new String[3]);

        // Verify if all VMs are on and can connect to their server.
        // (the one invoking this test doesn't need to run the server as the unit tests will do it automatically).
        // This test will fail if the VMs are off or are not running the servers
        assertHostsOn(myErr);

        // Nothing should be outputted by grep if exited already
        assertFalse(myOut.toString().contains(INVALID_COMMAND_MESSAGE));

        myOut.close();
        myErr.close();
    }

    /**
     * Reset servers from test mode to actual read mode
     * 
     * @throws UnknownHostException -- If local host does not exit
     */
    @After 
    public void resetServersFromTests() throws UnknownHostException{
        String localhost = InetAddress.getLocalHost().getHostName();
        for (int i = 0; i < hostnames.size(); i++) {
            if (!hostnames.get(i).equals(localhost)) {
                Client client = new Client("resetTest\n", hostnames.get(i), new SynchronizedPrinter());
                client.start();
            }
        }
    }

}