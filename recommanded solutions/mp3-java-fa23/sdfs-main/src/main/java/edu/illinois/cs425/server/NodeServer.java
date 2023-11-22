package edu.illinois.cs425.server;

import edu.illinois.cs425.service.HeartbeatService;
import edu.illinois.cs425.service.SDFSInternalService;
import edu.illinois.cs425.service.SDFSNodeService;
import edu.illinois.cs425.utils.Constants;
import io.grpc.ManagedChannel;
import io.grpc.Server;
import io.grpc.ServerBuilder;

import java.io.IOException;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.concurrent.ConcurrentHashMap;

public class NodeServer {
    public static void main(String[] args) throws UnknownHostException, SocketException {
        final String ip = InetAddress.getLocalHost().getHostAddress();
        final int port = Constants.PORT;
        ConcurrentHashMap<String, ManagedChannel> sharedChannels = new ConcurrentHashMap<>();
        HeartbeatService heartbeatService = new HeartbeatService(Integer.parseInt(args[0]), ip, port, System.currentTimeMillis(),sharedChannels);
        new HeartbeatServer(heartbeatService).start();
        Server server = ServerBuilder
                .forPort(port)
                .addService(heartbeatService)
                .addService(new SDFSInternalService())
                .addService(new SDFSNodeService(sharedChannels))
                .maxInboundMessageSize(Integer.MAX_VALUE)
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
            for (ManagedChannel channel : sharedChannels.values()) {
                channel.shutdown();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
