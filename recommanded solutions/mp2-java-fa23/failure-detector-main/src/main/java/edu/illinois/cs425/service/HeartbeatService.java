package edu.illinois.cs425.service;

import com.google.protobuf.Empty;
import edu.illinois.cs425.grpcServices.*;
import edu.illinois.cs425.grpcServices.HeartbeatServerGrpc;
import edu.illinois.cs425.utils.Constants;
import edu.illinois.cs425.utils.MembershipInfo;
import edu.illinois.cs425.utils.NodeInfo;
import edu.illinois.cs425.utils.NodeStatus;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import io.grpc.stub.StreamObserver;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;

public class HeartbeatService extends HeartbeatServerGrpc.HeartbeatServerImplBase {
    NodeInfo nodeInfo;
    private final Map<NodeInfo, HeartbeatServerGrpc.HeartbeatServerBlockingStub> heartbeatStubs;
    public ConcurrentHashMap<NodeInfo, MembershipInfo> membershipList; //nodeID, heartbeat counter

    public HeartbeatService(int index, String ip, int port, long timestamp) {
        this.nodeInfo = new NodeInfo(index, ip, port);
        this.nodeInfo.setTimestamp(timestamp);
        heartbeatStubs = new HashMap<>();
        membershipList = new ConcurrentHashMap<>();
        membershipList.put(this.nodeInfo, new MembershipInfo(1, NodeStatus.ACTIVE, 0, 0)); //add itself to the membership list
        new Thread(this::introduce).start();
        new Thread(this::sendHeartbeat).start();
        new Thread(this::checkStatus).start();
    }

    public void heartbeat(MembershipListMessage request) {
//         System.out.printf("Node %s received heartbeat\n", this.nodeInfo.getId());
        // On receipt, it is merged with local membership list
        for (int i = 0; i < request.getMembershipListCount(); i++) {
            NodeInfo nodeID = new NodeInfo(request.getMembershipList(i).getNodeId());
            int heartbeatCounter = request.getMembershipList(i).getHeartbeatCounter();
            int incarnation = request.getMembershipList(i).getIncarnation();
            NodeStatus status = NodeStatus.values()[request.getMembershipList(i).getStatus()];
            if (membershipList.containsKey(nodeID)) {
                //if the node is told by someone that it is suspicious, it will increment its incarnation
                if (nodeID.equals(this.nodeInfo)) {
                    if (Constants.SUSPICION.get()&& status.equals(NodeStatus.SUSPICIOUS)){
                        membershipList.get(nodeID).incrementIncarnation();
                        System.out.println("Increment incarnation");
                        continue;
                    }
                }

                if (heartbeatCounter <= membershipList.get(nodeID).getHeartbeatCounter()) {
                    continue;
                }
                membershipList.get(nodeID).setHeartbeatCounter(heartbeatCounter);
                switch (membershipList.get(nodeID).getNodeStatus()) {
                    case ACTIVE:
                        if (Constants.SUSPICION.get()) {
                            if (status.equals(NodeStatus.ACTIVE)) {
                                membershipList.get(nodeID).setNodeTimeout(0);
                                membershipList.get(nodeID).setIncarnation(Math.max(incarnation, membershipList.get(nodeID).getIncarnation()));
                            } else if (status.equals(NodeStatus.SUSPICIOUS)) {
                                if (incarnation >= membershipList.get(nodeID).getIncarnation()) {
                                    membershipList.get(nodeID).setNodeTimeout(Constants.TIME_SUSPICION);
                                    membershipList.get(nodeID).setIncarnation(incarnation);
                                    membershipList.get(nodeID).setNodeStatus(NodeStatus.SUSPICIOUS);
                                }
                            }
                            else {
                                if (incarnation >= membershipList.get(nodeID).getIncarnation()) {
                                    membershipList.get(nodeID).setNodeTimeout(Constants.TIME_SUSPICION + Constants.TIME_FAIL);
                                    membershipList.get(nodeID).setIncarnation(incarnation);
                                    membershipList.get(nodeID).setNodeStatus(NodeStatus.FAILED);
                                }
                            }
                        }
                        else {
                            membershipList.get(nodeID).setHeartbeatCounter(heartbeatCounter);
                            membershipList.get(nodeID).setNodeTimeout(0);
                        }
                        break;
                    case SUSPICIOUS:
                        if (Constants.SUSPICION.get()) {
                            if (status.equals(NodeStatus.ACTIVE)) {
                                if (incarnation > membershipList.get(nodeID).getIncarnation()) {
                                    membershipList.get(nodeID).setNodeTimeout(0);
                                    membershipList.get(nodeID).setIncarnation(incarnation);
                                    membershipList.get(nodeID).setNodeStatus(NodeStatus.ACTIVE);
                                }
                            }
                            else if (status.equals(NodeStatus.SUSPICIOUS)) {
                                if (incarnation > membershipList.get(nodeID).getIncarnation()) {
                                    membershipList.get(nodeID).setNodeTimeout(Constants.TIME_SUSPICION);
                                    membershipList.get(nodeID).setIncarnation(incarnation);
                                }
                            }
                            else { // FAILED
                                if (incarnation >= membershipList.get(nodeID).getIncarnation()) {
                                    membershipList.get(nodeID).setNodeTimeout(Constants.TIME_SUSPICION + Constants.TIME_FAIL);
                                    membershipList.get(nodeID).setIncarnation(incarnation);
                                    membershipList.get(nodeID).setNodeStatus(NodeStatus.FAILED);
                                }
                            }
                        }
                        break;
                    case FAILED:
                        break;
                }
            }
            else {
                if (Constants.SUSPICION.get()) {
                    if (status.equals(NodeStatus.ACTIVE)) {
                        membershipList.put(nodeID, new MembershipInfo(heartbeatCounter, NodeStatus.ACTIVE, 0, incarnation));
                    } else if (status.equals(NodeStatus.SUSPICIOUS)) {
                        membershipList.put(nodeID, new MembershipInfo(heartbeatCounter, NodeStatus.SUSPICIOUS, Constants.TIME_SUSPICION, incarnation));
                    }
                }
                else {
                    membershipList.put(nodeID, new MembershipInfo(heartbeatCounter, NodeStatus.ACTIVE, 0, incarnation));
                }
                System.out.printf("New node %d is added to the membership list from heartbeat\n", nodeID.getId());
            }
        }
    }

    //Introduce server logic
    //Get: the membership entry from the new node
    //Gossip: send only active entry
    //Gossip + suspicion: send active and suspicious entry
    @Override
    public void introduce(IntroduceRequest request, StreamObserver<MembershipListMessage> responseObserver) {
        //Return the non fail entry to the new node
        System.out.println("Introduce request received");
        MembershipListMessage.Builder membershipListMessage =  MembershipListMessage.newBuilder();
        for (NodeInfo nodeInfo: membershipList.keySet()) {
            if (!membershipList.get(nodeInfo).getNodeStatus().equals(NodeStatus.FAILED)|| Constants.SUSPICION.get()) {
                membershipListMessage.addMembershipList(constructMembershipEntry(nodeInfo));
            }
        }
        responseObserver.onNext(membershipListMessage.build());
        responseObserver.onCompleted();
        //Add the new node to the membership list
        NodeInfo nodeID = new NodeInfo(request.getMembershipEntry().getNodeId());
        this.membershipList.put(nodeID, new MembershipInfo(
                request.getMembershipEntry().getHeartbeatCounter(),
                NodeStatus.values()[request.getMembershipEntry().getStatus()],
                0,
                request.getMembershipEntry().getIncarnation()));
    }

    @Override
    public void getMembershipList(Empty request, StreamObserver<MembershipListMessage> responseObserver) {
        MembershipListMessage.Builder membershipListMessage = MembershipListMessage.newBuilder();
        for (Map.Entry<NodeInfo, MembershipInfo> entry : membershipList.entrySet()) {
            membershipListMessage.addMembershipList(MembershipEntry.newBuilder()
                    .setStatus(entry.getValue().getNodeStatus().ordinal())
                    .setIncarnation(entry.getValue().getIncarnation())
                    .setNodeId(entry.getKey().toMessage())
                    .setHeartbeatCounter(entry.getValue().getHeartbeatCounter())
                    .build());
        }
        responseObserver.onNext(membershipListMessage.build());
        responseObserver.onCompleted();
    }

    @Override
    public void getNodeId(Empty request, StreamObserver<NodeId> responseObserver) {
        responseObserver.onNext(nodeInfo.toMessage());
        responseObserver.onCompleted();
    }

    @Override
    public void leave(Empty request, StreamObserver<Empty> responseObserver) {
        for (NodeInfo nodeInfo: membershipList.keySet()) {
            if (nodeInfo.equals(this.nodeInfo)) continue;
            if (!membershipList.get(nodeInfo).getNodeStatus().equals(NodeStatus.FAILED)) {
                try{
                    getDataStub(nodeInfo).leaveLocal(nodeInfo.toMessage());
                }catch (Exception e){
                    System.out.printf("Error occurs while leaving node %s\n", nodeInfo.getIp());
                }
            }
        }
        responseObserver.onNext(Empty.newBuilder().build());
        responseObserver.onCompleted();
    }

    @Override
    public void leaveLocal(NodeId nodeId, StreamObserver<Empty> responseObserver) {
        NodeInfo nodeInfo = new NodeInfo(nodeId);
        if (!membershipList.containsKey(nodeInfo)&& !membershipList.get(nodeInfo).getNodeStatus().equals(NodeStatus.FAILED)) {
            membershipList.get(nodeInfo).setNodeStatus(NodeStatus.FAILED);
            if (Constants.SUSPICION.get()){
                membershipList.get(nodeInfo).setNodeTimeout(Constants.TIME_FAIL + Constants.TIME_SUSPICION);
            } else {
                membershipList.get(nodeInfo).setNodeTimeout(Constants.TIME_FAIL);
            }
        }

        System.out.printf("Node %s is removed from the membership list.\n", nodeInfo);
        responseObserver.onNext(Empty.newBuilder().build());
        responseObserver.onCompleted();
    }

    @Override
    public void suspicion(SuspicionRequest request, StreamObserver<Empty> responseObserver) {
        Constants.SUSPICION.set(request.getSuspicion());
        responseObserver.onNext(Empty.newBuilder().build());
        responseObserver.onCompleted();
    }

    //Introduce client logic
    //Send: its own membership entry to the introducer
    //Get: the membership list from the introducer, and put into its own membership list
    public void introduce()  {
        String localHostName = null;
        try {
            localHostName = InetAddress.getLocalHost().getHostName();
        } catch (UnknownHostException e) {
            System.out.println("Error occurs while getting local host name");
        }
        if (!Objects.equals(localHostName, Constants.INTRODUCER_IP)) { //Introduce does not need to introduce itself
            HeartbeatServerGrpc.HeartbeatServerBlockingStub dataServerBlockingStub = getDataStub(
                    new NodeInfo(Constants.INTRODUCER_INDEX, Constants.INTRODUCER_IP, Constants.PORT)); //Every node knows the introducer's IP and port
            try {
                MembershipListMessage membershipListMessage =  dataServerBlockingStub.introduce(IntroduceRequest.newBuilder()
                        .setMembershipEntry(constructMembershipEntry(this.nodeInfo)).build());
                System.out.println("Introduce request sent and get back result");
                for (MembershipEntry membershipEntry: membershipListMessage.getMembershipListList()){
                    NodeStatus status = NodeStatus.values()[membershipEntry.getStatus()];
                    if (status.equals(NodeStatus.ACTIVE)) {
                        this.membershipList.put(new NodeInfo(membershipEntry.getNodeId()), new MembershipInfo(membershipEntry.getHeartbeatCounter(), NodeStatus.values()[membershipEntry.getStatus()], 0, membershipEntry.getIncarnation()));
                        System.out.printf("New node %s is added to the membership list\n", membershipEntry.getNodeId().getNodeIndex());
                    } else if (status.equals(NodeStatus.SUSPICIOUS)) {
                        if (Constants.SUSPICION.get()) {
                            this.membershipList.put(new NodeInfo(membershipEntry.getNodeId()), new MembershipInfo(membershipEntry.getHeartbeatCounter(), NodeStatus.values()[membershipEntry.getStatus()], Constants.TIME_SUSPICION, membershipEntry.getIncarnation()));
                        }
                    }
                }
            } catch (Exception e) {
                System.out.printf("Error occurs while introducing node %s\n", this.nodeInfo.getIp());
            }
        }
        System.out.println("Introduce finished");
    }

    //Nodes periodically send heartbeat to other nodes
    //Gossip: send only active entry
    //Gossip + suspicion: send all entry
    private void sendHeartbeat() {
        System.out.printf("Node %s starts to send heartbeat.\n", this.nodeInfo.getIp());
        while (true) {
            //increment the heartbeat counter
            membershipList.get(this.nodeInfo).incrementHeartbeatCounter();
            //construct heartbeat request
            MembershipListMessage.Builder builder = MembershipListMessage.newBuilder();
            for (NodeInfo nodeID : membershipList.keySet()) {
                if(membershipList.get(nodeID).getNodeStatus() == NodeStatus.FAILED && !Constants.SUSPICION.get()) {
                    continue;
                }
                builder.addMembershipList(constructMembershipEntry(nodeID));
            }
            MembershipListMessage membershipListMessage = builder.build();
            //randomly choose f percent nodes to send heartbeat
            int numberOfTargets = (int) Math.ceil(membershipList.size() * Constants.f);
            List<Object> neighborIds = Arrays.asList(membershipList.keySet().toArray());
            Collections.shuffle(neighborIds);
            for (int i = 0; i < numberOfTargets; i++) {
                if (i >= membershipList.size()) break;
                NodeInfo nodeId = (NodeInfo) neighborIds.get(i);
                if (nodeId.equals(this.nodeInfo) || membershipList.get(nodeId).getNodeStatus().equals(NodeStatus.FAILED)) {
                    numberOfTargets++;
                    continue;
                }
                try {
                    DatagramSocket socket = new DatagramSocket();
                    InetAddress address = InetAddress.getByName(nodeId.getIp());
                    byte[] buf = membershipListMessage.toByteArray();
                    DatagramPacket packet
                            = new DatagramPacket(buf, buf.length, address, Constants.PORT_UDP);
                    if (Math.random() >= Constants.DROP_RATE) {
                        socket.send(packet);
                    }
                } catch (Exception e) {
                    System.out.printf("Error occurred at node %s\n", this.nodeInfo.getIp());
                }
            }
            try {
                Thread.sleep(Constants.TIME_GOSSIP);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private HeartbeatServerGrpc.HeartbeatServerBlockingStub getDataStub(NodeInfo nodeId) {
        synchronized (heartbeatStubs) {
            if (!heartbeatStubs.containsKey(nodeId)) {
                ManagedChannel managedChannel = ManagedChannelBuilder
                        .forAddress(nodeId.getIp(), nodeId.getPort())
                        .usePlaintext().keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                        .build();
                HeartbeatServerGrpc.HeartbeatServerBlockingStub dataServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(managedChannel);
                heartbeatStubs.put(nodeId, dataServerBlockingStub);
            }
        }
        return heartbeatStubs.get(nodeId);
    }

    //Nodes periodically check the status of other nodes
    //Tsuspicion, Tfail, Tcleanup
    private void checkStatus() {
        System.out.println("Node starts to check status.");
        while (true) {
            for (NodeInfo nodeID : membershipList.keySet()) {
                int timeout = membershipList.get(nodeID).getNodeTimeout();
                switch (membershipList.get(nodeID).getNodeStatus()) {
                    case ACTIVE:
                        if (Constants.SUSPICION.get()) {
                            if (timeout > Constants.TIME_SUSPICION) {
                                membershipList.get(nodeID).setNodeStatus(NodeStatus.SUSPICIOUS);
                                System.out.printf("Node %s is marked as suspected at %d.\n", nodeID.getId(), System.currentTimeMillis());
                            }
                        } else {
                            if (timeout > Constants.TIME_FAIL) {
                                membershipList.get(nodeID).setNodeStatus(NodeStatus.FAILED);
                                System.out.printf("Node %s is marked as failed at %d.\n", nodeID.getId(), System.currentTimeMillis());
                            }
                        }
                        break;
                    case SUSPICIOUS:
                        if (Constants.SUSPICION.get()) {
                            if (timeout > Constants.TIME_SUSPICION + Constants.TIME_FAIL) {
                                membershipList.get(nodeID).setNodeStatus(NodeStatus.FAILED);
                                System.out.printf("Node %s is marked as failed at %d.\n", nodeID.getId(), System.currentTimeMillis());
                            }
                        } else {
                            membershipList.get(nodeID).setNodeStatus(NodeStatus.FAILED);
                            System.out.printf("Node %s is marked as failed at %d.\n", nodeID.getId(), System.currentTimeMillis());
                        }
                        break;
                    case FAILED:
                        if (Constants.SUSPICION.get()) {
                            if (timeout > Constants.TIME_SUSPICION + Constants.TIME_FAIL + Constants.TIME_CLEANUP) {
                                membershipList.remove(nodeID);
                                System.out.printf("Node %s is removed from the membership list at %d.\n", nodeID.getId(), System.currentTimeMillis());
                            }
                        } else {
                            if (timeout > Constants.TIME_FAIL + Constants.TIME_CLEANUP) {
                                membershipList.remove(nodeID);
                                System.out.printf("Node %s is removed from the membership list at %d.\n", nodeID.getId(), System.currentTimeMillis());
                            }
                        }
                        break;
                }
                if (membershipList.containsKey(nodeID) && !nodeID.equals(this.nodeInfo)) {
                    membershipList.get(nodeID).incrementNodeTimeout(Constants.TIME_CHECK);
                }
            }
            try {
                Thread.sleep(Constants.TIME_CHECK);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            for (NodeInfo nodeID : membershipList.keySet()) {
                System.out.printf("%d:| %d:%d:%s\n",
                        nodeID.getId(),
                        membershipList.get(nodeID).getHeartbeatCounter(),
                        membershipList.get(nodeID).getNodeTimeout(),
                        membershipList.get(nodeID).getNodeStatus());
            }
        }
    }

    private MembershipEntry constructMembershipEntry(NodeInfo nodeInfo) {
        return MembershipEntry.newBuilder()
                .setNodeId(nodeInfo.toMessage())
                .setHeartbeatCounter(membershipList.get(nodeInfo).getHeartbeatCounter())
                .setStatus(membershipList.get(nodeInfo).getNodeStatus().ordinal())
                .setIncarnation(membershipList.get(nodeInfo).getIncarnation())
                .build();
    }
}
