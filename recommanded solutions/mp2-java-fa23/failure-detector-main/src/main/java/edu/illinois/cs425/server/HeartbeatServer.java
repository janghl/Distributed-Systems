package edu.illinois.cs425.server;

import com.google.protobuf.InvalidProtocolBufferException;
import edu.illinois.cs425.grpcServices.MembershipListMessage;
import edu.illinois.cs425.service.HeartbeatService;
import edu.illinois.cs425.utils.Constants;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.util.Arrays;

public class HeartbeatServer extends Thread {
    private DatagramSocket socket;
    private boolean running;
    private byte[] buf = new byte[1024];

    private HeartbeatService heartbeatService;

    public HeartbeatServer(HeartbeatService heartbeatService) throws SocketException {
        socket = new DatagramSocket(Constants.PORT_UDP);
        this.heartbeatService = heartbeatService;
    }

    public void run() {
        running = true;

        while (running) {
            DatagramPacket packet
                    = new DatagramPacket(buf, buf.length);
            try {
                socket.receive(packet);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
            MembershipListMessage membershipListMessage = null;
            try {
                membershipListMessage = MembershipListMessage.parseFrom(Arrays.copyOfRange(packet.getData(), 0, packet.getLength()));
                heartbeatService.heartbeat(membershipListMessage);
            } catch (InvalidProtocolBufferException e) {
                throw new RuntimeException(e);
            }
        }
        socket.close();
    }
}
