package edu.illinois.cs425.service;

import com.google.protobuf.Empty;
import edu.illinois.cs425.grpcServices.*;
import edu.illinois.cs425.utils.*;
import io.grpc.Channel;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import io.grpc.stub.StreamObserver;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.stream.Collectors;

public class HeartbeatService extends HeartbeatServerGrpc.HeartbeatServerImplBase {
    NodeInfo nodeInfo;
    private ConcurrentHashMap<String, ManagedChannel> sharedChannels;
    public ConcurrentHashMap<NodeInfo, MembershipInfo> membershipList; //nodeID, heartbeat counter

    //=======================leader election=======================
    private int currentTerm = 0;
    private int votedTerm = 0;
    private int votedFor = 0;
    private final Object statusLock = new Object();

    private LeaderStatus leaderStatus = LeaderStatus.LEADER;

    //======================leader service=========================
    ConcurrentHashMap<String, List<ShardReplicaInfo>> index;
    ConcurrentHashMap<String, ReadWriteLock> locks;

    public HeartbeatService(int index, String ip, int port, long timestamp, Map<String, ManagedChannel> sharedChannels) {
        this.nodeInfo = new NodeInfo(index, ip, port);
        this.nodeInfo.setTimestamp(timestamp);
        this.sharedChannels= new ConcurrentHashMap<>(sharedChannels);
        membershipList = new ConcurrentHashMap<>();
        membershipList.put(this.nodeInfo, new MembershipInfo(1, NodeStatus.ACTIVE, 0, 0)); //add itself to the membership list
        this.index = new ConcurrentHashMap<>();
        this.locks = new ConcurrentHashMap<>();
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
                                    nodeFailed(nodeID);
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
                                    nodeFailed(nodeID);
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
//                System.out.printf("New node %d is added to the membership list from heartbeat\n", nodeID.getId());
            }
        }
    }

    //Introduce server logic
    //Get: the membership entry from the new node
    //Gossip: send only active entry
    //Gossip + suspicion: send active and suspicious entry
    @Override
    public void introduce(IntroduceRequest request, StreamObserver<IntroducerResponse> responseObserver) {
        //Return the non fail entry to the new node
//        System.out.println("Introduce request received");
        IntroducerResponse.Builder introducerResponse = IntroducerResponse.newBuilder();
        for (NodeInfo nodeInfo: membershipList.keySet()) {
            if (!membershipList.get(nodeInfo).getNodeStatus().equals(NodeStatus.FAILED)|| Constants.SUSPICION.get()) {
                introducerResponse.addMembershipList(constructMembershipEntry(nodeInfo));
            }
        }
        introducerResponse.setLeaderIndex(Constants.LEADER_INDEX);
        introducerResponse.setTerm(this.currentTerm);
        responseObserver.onNext(introducerResponse.build());
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
    public void getLeaderIndex(Empty request, StreamObserver<LeaderIndex> responseObserver) {
        responseObserver.onNext(LeaderIndex.newBuilder()
                .setLeaderIndex(Constants.LEADER_INDEX)
                .build());
        responseObserver.onCompleted();
    }

    @Override
    public void leave(Empty request, StreamObserver<Empty> responseObserver) {
        for (NodeInfo nodeInfo: membershipList.keySet()) {
            if (nodeInfo.equals(this.nodeInfo)) continue;
            if (!membershipList.get(nodeInfo).getNodeStatus().equals(NodeStatus.FAILED)) {
                try{
                    HeartbeatServerGrpc.newBlockingStub(getSharedChannels(nodeInfo.getIp(), nodeInfo.getPort())).leaveLocal(nodeInfo.toMessage());
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
            nodeFailed(nodeInfo);
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
            HeartbeatServerGrpc.HeartbeatServerBlockingStub dataServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(getSharedChannels(Constants.INTRODUCER_IP,Constants.PORT)); //Every node knows the introducer's IP and port
            try {
                IntroducerResponse introducerResponse =  dataServerBlockingStub.introduce(IntroduceRequest.newBuilder()
                        .setMembershipEntry(constructMembershipEntry(this.nodeInfo)).build());
                this.currentTerm = introducerResponse.getTerm();
                this.votedTerm = introducerResponse.getTerm();
                Constants.LEADER_INDEX = introducerResponse.getLeaderIndex();
//                System.out.println("Introduce request sent and get back result");
                for (MembershipEntry membershipEntry: introducerResponse.getMembershipListList()){
                    NodeStatus status = NodeStatus.values()[membershipEntry.getStatus()];
                    if (status.equals(NodeStatus.ACTIVE)) {
                        this.membershipList.put(new NodeInfo(membershipEntry.getNodeId()), new MembershipInfo(membershipEntry.getHeartbeatCounter(), NodeStatus.values()[membershipEntry.getStatus()], 0, membershipEntry.getIncarnation()));
//                        System.out.printf("New node %s is added to the membership list\n", membershipEntry.getNodeId().getNodeIndex());
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
//        System.out.println("Introduce finished");
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

    private ManagedChannel getSharedChannels(String ip, int port) {
        synchronized (sharedChannels) {
            if (!sharedChannels.containsKey(ip)) {
                ManagedChannel managedChannel = ManagedChannelBuilder.forAddress(ip,port)
                .usePlaintext().keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                .build();
                sharedChannels.put(ip, managedChannel);
            }
        }
        return sharedChannels.get(ip);
    }

    //Nodes periodically check the status of other nodes
    //Tsuspicion, Tfail, Tcleanup
    private void checkStatus() {
//        System.out.println("Node starts to check status.");
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
                                nodeFailed(nodeID);
                                if (nodeID.getId() == Constants.LEADER_INDEX){
                                    leaderElection();
                                }
                            }
                        }
                        break;
                    case SUSPICIOUS:
                        if (Constants.SUSPICION.get()) {
                            if (timeout > Constants.TIME_SUSPICION + Constants.TIME_FAIL) {
                                membershipList.get(nodeID).setNodeStatus(NodeStatus.FAILED);
                                System.out.printf("Node %s is marked as failed at %d.\n", nodeID.getId(), System.currentTimeMillis());
                                nodeFailed(nodeID);
                                if (nodeID.getId() == Constants.LEADER_INDEX) {
                                    leaderElection();
                                }
                            }
                        } else {
                            membershipList.get(nodeID).setNodeStatus(NodeStatus.FAILED);
                            System.out.printf("Node %s is marked as failed at %d.\n", nodeID.getId(), System.currentTimeMillis());
                            nodeFailed(nodeID);
                            if (nodeID.getId() == Constants.LEADER_INDEX){
                                leaderElection();
                            }
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
//            for (NodeInfo nodeID : membershipList.keySet()) {
//                System.out.printf("%d:| %d:%d:%s\n",
//                        nodeID.getId(),
//                        membershipList.get(nodeID).getHeartbeatCounter(),
//                        membershipList.get(nodeID).getNodeTimeout(),
//                        membershipList.get(nodeID).getNodeStatus());
//            }
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

    //=======================================Leader Election=======================================
    private void leaderElection() {
        this.leaderStatus = LeaderStatus.NON_LEADER;
        while(this.leaderStatus == LeaderStatus.NON_LEADER){
            System.out.println("Try leader election");
            try {
                Thread.sleep(new Random().nextInt(Constants.LEADER_TIMEOUT));
            }catch (InterruptedException e) {
                e.printStackTrace();
            }
            if (this.currentTerm < this.votedTerm){ // there is a leader election process in the network
                this.currentTerm = this.votedTerm;
                continue;
            }
            if (this.leaderStatus == LeaderStatus.LEADER){ //have leader
                return;
            }
            System.out.println("A true new run begins");
            int grantVotes = 0;
            this.currentTerm++;
            for (NodeInfo nodeInfo: this.membershipList.keySet()) {
                if (!membershipList.get(nodeInfo).getNodeStatus().equals(NodeStatus.FAILED)) {
                    try{
                        VoteResponse voteResponse = HeartbeatServerGrpc.newBlockingStub(getSharedChannels(nodeInfo.getIp(),nodeInfo.getPort())).requestVote(VoteRequest.newBuilder()
                                .setCandidateIndex(this.nodeInfo.getId())
                                .setTerm(this.currentTerm)
                                .build());
                        if (this.currentTerm == voteResponse.getTerm() && voteResponse.getVoteGranted()) {
                            grantVotes++;
                        }
                    } catch (Exception e){
                        System.out.printf("Error occurs while request vote from node %s\n", nodeInfo.getIp());
                    }
                    if(grantVotes > Constants.CLUSTER_SIZE / 2) {
                        System.out.printf("Node %d is elected as the leader.\n", this.nodeInfo.getId());
                        //confirm the leader
                        for (NodeInfo nodeInfo1: membershipList.keySet()) {
                            if (!membershipList.get(nodeInfo1).getNodeStatus().equals(NodeStatus.FAILED)) {
                                HeartbeatServerGrpc.newBlockingStub(getSharedChannels(nodeInfo1.getIp(),nodeInfo1.getPort())).confirmLeader(ConfirmLeaderRequest.newBuilder()
                                        .setLeaderIndex(this.nodeInfo.getId())
                                        .setLeaderIp(this.nodeInfo.getIp())
                                        .setTerm(this.currentTerm)
                                        .build());
                            }
                        }
                        initiateLeader();
                        return;
                    }
                }
            }
        }
    }

    @Override
    public void requestVote(VoteRequest request, StreamObserver<VoteResponse> responseObserver) {
        boolean voteGranted = false;
        synchronized (this.statusLock) {
            if (request.getTerm() > this.votedTerm) {
                this.votedFor = request.getCandidateIndex();
                this.votedTerm = request.getTerm();
                voteGranted = true;
                System.out.printf("Vote for %d\n", request.getCandidateIndex());
            }
        }
        responseObserver.onNext(VoteResponse.newBuilder()
                .setTerm(request.getTerm())
                .setVoteGranted(voteGranted)
                .build());
        responseObserver.onCompleted();
    }

    @Override
    public void confirmLeader(ConfirmLeaderRequest request, StreamObserver<Empty> responseObserver) {
        System.out.println("Confirm leader request received");
        Constants.LEADER_INDEX = request.getLeaderIndex();
        Constants.LEADER_IP = request.getLeaderIp();
        this.votedTerm = request.getTerm();
        this.currentTerm = request.getTerm();
        this.leaderStatus = LeaderStatus.LEADER;
        responseObserver.onNext(Empty.newBuilder().build());
        responseObserver.onCompleted();
    }

    //TODO: new leader ask network...
    public void initiateLeader() {
        ExecutorService executorService = Executors.newFixedThreadPool(Constants.NUM_WORKERS);
        ConcurrentHashMap<ShardInfo, List<NodeInfo>> map = new ConcurrentHashMap<>();
        for (NodeInfo nodeInfo1: membershipList.keySet()) {
            if (membershipList.get(nodeInfo1).getNodeStatus() != NodeStatus.ACTIVE) continue;
            //Ask each node to send its indices
            executorService.submit(() -> {
                ManagedChannel managedChannel = ManagedChannelBuilder
                        .forAddress(nodeInfo1.getIp(), nodeInfo1.getPort())
                        .usePlaintext()
                        .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                        .build();
                SDFSInternalServiceGrpc.SDFSInternalServiceBlockingStub sdfsInternalServiceBlockingStub =
                        SDFSInternalServiceGrpc.newBlockingStub(managedChannel);
                GetShardInfoResponse getShardInfoResponse = sdfsInternalServiceBlockingStub.getShardInfo(Empty.newBuilder().build());
                for (ShardInfo shardInfo: getShardInfoResponse.getShardInfosList()) {
                    map.putIfAbsent(shardInfo, new ArrayList<>());
                    map.get(shardInfo).add(nodeInfo1);
                }
                managedChannel.shutdown();
            });
        }
        try {
            executorService.shutdown();
            executorService.awaitTermination(Long.MAX_VALUE, TimeUnit.NANOSECONDS);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        index = new ConcurrentHashMap<>();
        for (ShardInfo shardInfo : map.keySet()) {
            locks.putIfAbsent(shardInfo.getSdfsFileName(), new ReentrantReadWriteLock(true));
            index.putIfAbsent(shardInfo.getSdfsFileName(), new ArrayList<>());
            index.get(shardInfo.getSdfsFileName()).add(new ShardReplicaInfo(shardInfo.getSdfsFileName(),shardInfo.getShardId(), map.get(shardInfo)));
        }
    }

    //TODO: leader checking process

    //===============================leader service=================================
    @Override
    public void initiateGetLeader(InitiateGetRequestLeader request, StreamObserver<InitiateGetResponseLeader> responseObserver) {
        if (!locks.containsKey(request.getSdfsFileName())){
            responseObserver.onNext(InitiateGetResponseLeader.newBuilder().build());
            responseObserver.onCompleted();
            return;
        }
        locks.get(request.getSdfsFileName()).readLock().lock();
        InitiateGetResponseLeader initiateGetResponseLeader = InitiateGetResponseLeader.newBuilder()
                .addAllShards(index.get(request.getSdfsFileName()).stream().map(ShardReplicaInfo::toMessage).collect(Collectors.toList()))
                .build();
        responseObserver.onNext(initiateGetResponseLeader);
        responseObserver.onCompleted();
    }

    @Override
    public void getFileLocationLeader(InitiateGetRequestLeader request, StreamObserver<InitiateGetResponseLeader> responseObserver) {
        InitiateGetResponseLeader initiateGetResponseLeader = InitiateGetResponseLeader.newBuilder()
                .addAllShards(index.get(request.getSdfsFileName()).stream().map(ShardReplicaInfo::toMessage).collect(Collectors.toList()))
                .build();
        responseObserver.onNext(initiateGetResponseLeader);
        responseObserver.onCompleted();
    }

    @Override
    public void finishGetLeader(FinishGetRequestLeader request, StreamObserver<FinishGetResponseLeader> responseObserver) {
        if (!locks.containsKey(request.getSdfsFileName())){
            responseObserver.onNext(FinishGetResponseLeader.newBuilder().setSuccess(true).build());
            responseObserver.onCompleted();
            return;
        }
        locks.get(request.getSdfsFileName()).readLock().unlock();
        FinishGetResponseLeader finishGetResponseLeader = FinishGetResponseLeader.newBuilder().setSuccess(true).build();
        responseObserver.onNext(finishGetResponseLeader);
        responseObserver.onCompleted();
    }

    @Override
    public void initiateDeleteLeader(InitiateDeleteRequestLeader request, StreamObserver<InitiateDeleteResponseLeader> responseObserver) {
        if (!locks.containsKey(request.getSdfsFileName())){
            responseObserver.onNext(InitiateDeleteResponseLeader.newBuilder().build());
            responseObserver.onCompleted();
            return;
        }
        InitiateDeleteResponseLeader initiateDeleteResponseLeader = InitiateDeleteResponseLeader.newBuilder()
                .addAllShards(index.get(request.getSdfsFileName()).stream().map(ShardReplicaInfo::toMessage).collect(Collectors.toList())).build();
        locks.get(request.getSdfsFileName()).writeLock().lock();
        index.remove(request.getSdfsFileName());
        responseObserver.onNext(initiateDeleteResponseLeader);
        responseObserver.onCompleted();
    }

    @Override
    public void finishDeleteLeader(FinishDeleteRequestLeader request, StreamObserver<FinishDeleteResponseLeader> responseObserver) {
        if (!locks.containsKey(request.getSdfsFileName())){
            responseObserver.onNext(FinishDeleteResponseLeader.newBuilder().setSuccess(true).build());
            responseObserver.onCompleted();
            return;
        }
        locks.get(request.getSdfsFileName()).writeLock().unlock();
        locks.remove(request.getSdfsFileName());
        FinishDeleteResponseLeader finishDeleteResponseLeader = FinishDeleteResponseLeader.newBuilder().setSuccess(true).build();
        responseObserver.onNext(finishDeleteResponseLeader);
        responseObserver.onCompleted();
    }

    @Override
    public void initiatePutLeader(InitiatePutRequestLeader request, StreamObserver<InitiatePutResponseLeader> responseObserver) {
        locks.putIfAbsent(request.getSdfsFileName(), new ReentrantReadWriteLock(true));
        locks.get(request.getSdfsFileName()).writeLock().lock();
        List<NodeInfo> nodeInfos = new ArrayList<>(membershipList.keySet());
        int nodesCount = nodeInfos.size();
        int numShards = (int) Math.ceil(request.getFileSize() * 1.0 / Constants.SHARD_SIZE);
        InitiatePutResponseLeader.Builder builder = InitiatePutResponseLeader.newBuilder();
        System.out.println("Initiate put leader 625");
        if (index.containsKey(request.getSdfsFileName())) {
            // overwrite
            System.out.println("Initiate put leader 628");
            if (index.get(request.getSdfsFileName()).size() > numShards) {
                // if the later file is smaller than the previous one, remove the extra shards
                index.get(request.getSdfsFileName()).subList(numShards, index.get(request.getSdfsFileName()).size()).clear();
                builder.addAllShards(index.get(request.getSdfsFileName()).stream().map(ShardReplicaInfo::toMessage).collect(Collectors.toList()));
                responseObserver.onNext(builder.build());
                responseObserver.onCompleted();
                return;
            }
            builder.addAllShards(index.get(request.getSdfsFileName()).stream().map(ShardReplicaInfo::toMessage).collect(Collectors.toList()));
        }
        System.out.println("Initiate put leader 637");
        for (int i = builder.getShardsCount(); i < numShards; i++) {
            System.out.println("number of shared :" + i);
            ShardReplicaLocations.Builder builder1 = ShardReplicaLocations.newBuilder();
            builder1.setShardId(i);
            builder1.setSdfsFileName(request.getSdfsFileName());
            HashSet<NodeId> nodeIds = new HashSet<>();
            while (nodeIds.size()< Constants.NUM_REPLICAS){
                int index;
                for (index = Constants.rand.nextInt(nodesCount); !membershipList.get(nodeInfos.get(index)).getNodeStatus().equals(NodeStatus.ACTIVE); index = Constants.rand.nextInt(nodesCount));
                nodeIds.add(nodeInfos.get(index).toMessage());
            }
            builder1.addAllNodeIds(nodeIds);
            builder.addShards(builder1.build());
        }
        responseObserver.onNext(builder.build());
        responseObserver.onCompleted();
    }

    @Override
    public void finishPutLeader(PutIndicesRequestLeader request, StreamObserver<PutIndicesResponseLeader> responseObserver) {
        index.put(request.getSdfsFileName(), new ArrayList<>());
        for (ShardReplicaLocations shard : request.getShardsList()) {
            index.get(request.getSdfsFileName()).add(new ShardReplicaInfo(shard));
        }
        PutIndicesResponseLeader putIndicesResponseLeader = PutIndicesResponseLeader.newBuilder().setSuccess(true).build();
        locks.get(request.getSdfsFileName()).writeLock().unlock();
        responseObserver.onNext(putIndicesResponseLeader);
        responseObserver.onCompleted();
    }

    private synchronized void nodeFailed(NodeInfo nodeInfo) {
        List<NodeInfo> nodeInfos = new ArrayList<>(membershipList.keySet());
        for (Map.Entry<String, List<ShardReplicaInfo>> entry : index.entrySet()) {
            for (ShardReplicaInfo shard : entry.getValue()) {
                HashSet<Integer> set = new HashSet<>();
                for (NodeInfo replica : shard.getReplicas()) {
                    if (nodeInfo.getId() != replica.getId()) {
                        set.add(replica.getId());
                    }
                }
                if (set.size() < Constants.NUM_REPLICAS) {
                    int newReplica;
                    int nodesCount = membershipList.size();
                    for (newReplica = Constants.rand.nextInt(nodesCount);
                         !(membershipList.get(nodeInfos.get(newReplica)).getNodeStatus().equals(NodeStatus.ACTIVE) && !set.contains(newReplica));
                         newReplica = Constants.rand.nextInt(nodesCount)){
                        System.out.println("failednewReplica: " + newReplica);
                    };
                    NodeInfo existingReplica = nodeInfos.get(set.iterator().next());
                    while (!membershipList.containsKey(existingReplica) || !membershipList.get(existingReplica).getNodeStatus().equals(NodeStatus.ACTIVE)){
                        existingReplica = nodeInfos.get(set.iterator().next());
                    }
                    SDFSNodeServiceGrpc.SDFSNodeServiceBlockingStub sdfsNodeServiceBlockingStub = SDFSNodeServiceGrpc.newBlockingStub(getSharedChannels(existingReplica.getIp(), existingReplica.getPort()));
                    ReplicateShardResponse replicateShardResponse = sdfsNodeServiceBlockingStub.replicateShard(ReplicateShardRequest.newBuilder().setNewReplica(nodeInfos.get(newReplica).toMessage()).setSdfsFileName(entry.getKey()).setShardId(shard.getShardId()).build());
                    shard.getReplicas().remove(nodeInfo);
                    shard.addReplica(nodeInfos.get(newReplica));
                    System.out.printf("Updated Index. Replicated Shard: %s", replicateShardResponse.getSuccess());
                }
            }
        }
    }
}
