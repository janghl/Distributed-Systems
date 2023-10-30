package edu.illinois.cs425.server;

import edu.illinois.cs425.grpcServices.HeartbeatServerGrpc;
import edu.illinois.cs425.service.HeartbeatService;
import edu.illinois.cs425.utils.Constants;
import edu.illinois.cs425.utils.MembershipInfo;
import edu.illinois.cs425.utils.NodeInfo;
import io.grpc.Server;
import io.grpc.ServerBuilder;
import java.io.IOException;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class NodeServer {
    public static void main(String[] args) throws UnknownHostException, SocketException {
        final String ip = InetAddress.getLocalHost().getHostAddress();
        final int port = Constants.PORT;
        HeartbeatService heartbeatService = new HeartbeatService(Integer.parseInt(args[0]), ip, port, System.currentTimeMillis());
        new HeartbeatServer(heartbeatService).start();
        Server server = ServerBuilder
                .forPort(port)
                .addService(heartbeatService)
                .build();
        try {
            server.start();
        }
        catch (IOException ex) {
            ex.printStackTrace();
        }
        System.out.println("Node Server started at " + server.getPort());
        System.out.println("Node Server Ip is " + ip);
        try {
            server.awaitTermination();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
