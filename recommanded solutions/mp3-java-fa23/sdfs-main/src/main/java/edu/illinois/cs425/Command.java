package edu.illinois.cs425;

import com.google.protobuf.ByteString;
import com.google.protobuf.Empty;
import edu.illinois.cs425.grpcServices.*;
import edu.illinois.cs425.utils.*;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import org.apache.commons.cli.*;

import java.io.PrintStream;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class Command {
    private static final ConcurrentMap<String, ManagedChannel> sharedChannel = new ConcurrentHashMap<>();
    public static void main(String[] args) {
        Options options = new Options();
        // Help and Version
        options.addOption("h", "help", false, "Output a help message and exit.");
        options.addOption("V", "version", false, "Output the version number and exit.");

        options.addOption("s", "suspicion", false, "Switch to suspicion mode");
        options.addOption("ns", "no-suspicion", false, "Switch to normal mode.");
        options.addOption("l", "leave", false, "Leave the cluster.");
        options.addOption("ml", "membership-list", false, "Get self membership list.");
        options.addOption("n", "node-id", false, "Get node id.");
        options.addOption("self", "list-self", false, "List self id.");
        options.addOption("ip", "ip", true, "Get node id.");
        options.addOption("p", "port", true, "Get node id.");

        //======================leader election=======================
        options.addOption("sl","show-leader",false,"Show leader");

        //======================sdfs operations=======================
        options.addOption("sdfsfilename", "sdfsfilename", true, "SDFS filename");
        options.addOption("localfilename", "localfilename", true, "Local filename");
        //put localfilename sdfsfilename
        options.addOption("put", "put", false, "Put localfilename sdfsfilename");
        //get sdfsfilename localfilename
        options.addOption("get", "get", false, "Get sdfsfilename localfilename");
        //delete sdfsfilename
        options.addOption("delete", "delete", false, "Delete sdfsfilename");
        //ls sdfsfilename: list all machine (VM) addresses where this file is currently being stored
        options.addOption("ls", "ls", false, "List all machine (VM) addresses where this file is currently being stored");
        //store: At any machine, list all files currently being stored at this machine.
        options.addOption("store", "store", false, "List all files currently being stored at this machine");
        Option multiread = new Option("multiread", "multiread", true, "Read a file from multiple machines");
        multiread.setArgs(Option.UNLIMITED_VALUES);
        multiread.setValueSeparator(',');
        options.addOption(multiread);


        CommandLineParser parser = new DefaultParser();
        try {
            // Parse the command line arguments
            CommandLine cmd = parser.parse(options, args);
            Option[] optionList = cmd.getOptions();
            for (Option option : optionList) {
                if (option.getLongOpt().equals("help")) {
                    HelpFormatter formatter = new HelpFormatter();
                    formatter.printHelp("help", options);
                    System.exit(0);
                } else if (option.getLongOpt().equals("version")) {
                    System.out.println("v0.0.1");
                    System.exit(0);
                } else {
                    String command = option.getLongOpt();
                    if (command.equals("suspicion")) {
                        boolean suspicion = true;
                        SuspicionRequest suspicionRequest = SuspicionRequest.newBuilder().setSuspicion(suspicion).build();
                        Map<Integer, NodeInfo> nodeInfoMap = Utils.readNodeConfig(args[0]);
                        for (Map.Entry<Integer, NodeInfo> entry : nodeInfoMap.entrySet()) {
                            String ip = entry.getValue().getIp();
                            int port = entry.getValue().getPort();
                            try {
                                ManagedChannel managedChannel = ManagedChannelBuilder
                                        .forAddress(ip, port)
                                        .usePlaintext()
                                        .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                                        .build();
                                HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(managedChannel);
                                heartbeatServerBlockingStub.suspicion(suspicionRequest);
                                managedChannel.shutdown();
                            }
                            catch (Exception ex) {

                            }
                            finally {
                                System.out.println(entry.getValue().getIp());
                            }
                        }
                    } else if (command.equals("no-suspicion")) {
                        boolean suspicion = false;
                        SuspicionRequest suspicionRequest = SuspicionRequest.newBuilder().setSuspicion(suspicion).build();
                        Map<Integer, NodeInfo> nodeInfoMap = Utils.readNodeConfig(args[0]);
                        for (Map.Entry<Integer, NodeInfo> entry : nodeInfoMap.entrySet()) {
                            String ip = entry.getValue().getIp();
                            int port = entry.getValue().getPort();
                            try {
                                ManagedChannel managedChannel = ManagedChannelBuilder
                                        .forAddress(ip, port)
                                        .usePlaintext()
                                        .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                                        .build();
                                HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(managedChannel);
                                heartbeatServerBlockingStub.suspicion(suspicionRequest);
                                managedChannel.shutdown();
                            }
                            catch (Exception ex) {

                            }
                            finally {
                                System.out.println(entry.getValue().getIp());
                            }
                        }
                    } else if (command.equals("leave")) {
                        String ip = cmd.getOptionValue("ip");
                        int port = Integer.parseInt(cmd.getOptionValue("port"));
                        try {
                            ManagedChannel managedChannel = ManagedChannelBuilder
                                    .forAddress(ip, port)
                                    .usePlaintext()
                                    .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                                    .build();
                            HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(managedChannel);
                            heartbeatServerBlockingStub.leave(Empty.newBuilder().build());
                            System.out.println(ip + "left");
                            managedChannel.shutdown();
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }
                    } else if (command.equals("membership-list")) {
                        String ip = "127.0.0.1";
                        int port = Constants.PORT;
                        try {
                            ManagedChannel managedChannel = ManagedChannelBuilder
                                    .forAddress(ip, port)
                                    .usePlaintext()
                                    .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                                    .build();
                            HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(managedChannel);
                            MembershipListMessage membershipListMessage = heartbeatServerBlockingStub.getMembershipList(Empty.newBuilder().build());
                            for (MembershipEntry membershipEntry : membershipListMessage.getMembershipListList()) {
                                System.out.printf("%d:%s:%d:%d| %d:%d:%s\n",
                                        membershipEntry.getNodeId().getNodeIndex(),
                                        membershipEntry.getNodeId().getNodeIp(),
                                        membershipEntry.getNodeId().getNodePort(),
                                        membershipEntry.getNodeId().getNodeTimestamp(),
                                        membershipEntry.getHeartbeatCounter(),
                                        membershipEntry.getIncarnation(),
                                        NodeStatus.values()[membershipEntry.getStatus()]);
                            }
                            managedChannel.shutdown();
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }
                    } else if (command.equals("node-id")) {
                        String ip = cmd.getOptionValue("ip");
                        int port = Integer.parseInt(cmd.getOptionValue("port"));
                        try {
                            ManagedChannel managedChannel = ManagedChannelBuilder
                                    .forAddress(ip, port)
                                    .usePlaintext()
                                    .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                                    .build();
                            HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(managedChannel);
                            NodeId nodeId = heartbeatServerBlockingStub.getNodeId(Empty.newBuilder().build());
                            System.out.printf("%d:%s:%d:%d\n",
                                    nodeId.getNodeIndex(),
                                    nodeId.getNodeIp(),
                                    nodeId.getNodePort(),
                                    nodeId.getNodeTimestamp());
                            managedChannel.shutdown();
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }
                    } else if (command.equals("list-self")) {
                        String ip = "127.0.0.1";
                        int port = Constants.PORT;
                        try {
                            ManagedChannel managedChannel = ManagedChannelBuilder
                                    .forAddress(ip, port)
                                    .usePlaintext()
                                    .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                                    .build();
                            HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(managedChannel);
                            NodeId nodeId = heartbeatServerBlockingStub.getNodeId(Empty.newBuilder().build());
                            System.out.printf("%d:%s:%d:%d\n",
                                    nodeId.getNodeIndex(),
                                    nodeId.getNodeIp(),
                                    nodeId.getNodePort(),
                                    nodeId.getNodeTimestamp());
                            managedChannel.shutdown();
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }
                    } else if (command.equals("show-leader")) {
                        String ip = "127.0.0.1";
                        int port = Constants.PORT;
                        try {
                            ManagedChannel managedChannel = ManagedChannelBuilder
                                    .forAddress(ip, port)
                                    .usePlaintext()
                                    .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                                    .build();
                            HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(managedChannel);
                            LeaderIndex leaderIndex = heartbeatServerBlockingStub.getLeaderIndex(Empty.newBuilder().build());
                            System.out.printf("The leader is %d\n", leaderIndex.getLeaderIndex());
                            managedChannel.shutdown();
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }
                    }
                    else if (command.equals("put")) {
                        String sdfsfilename = cmd.getOptionValue("sdfsfilename");
                        String localfilename = cmd.getOptionValue("localfilename");
                        String ip = "127.0.0.1";
                        int port = Constants.PORT;
                        long starTime = System.currentTimeMillis();
                        byte[] fileContent = Utils.readFile(localfilename);
                        try {
                            //Step1: initiate put: get the indices from leader
                            ManagedChannel managedChannel = getSharedChannel(ip, port);
                            SDFSNodeServiceGrpc.SDFSNodeServiceBlockingStub sdfsNodeServiceBlockingStub = SDFSNodeServiceGrpc.newBlockingStub(managedChannel);
                            assert fileContent != null;
                            InitiatePutResponse initiatePutResponse = sdfsNodeServiceBlockingStub.initiatePut(
                                    InitiatePutRequest.newBuilder()
                                            .setSdfsFileName(sdfsfilename)
                                            .setFileSize(fileContent.length)
                                            .build());
                            //Step2: put the file to the replicas
                            ExecutorService executorService = Executors.newFixedThreadPool(Constants.NUM_WORKERS);
                            for (ShardReplicaLocations eachShardLocation : initiatePutResponse.getShardsList()) {
                                for (NodeId nodeInfo : eachShardLocation.getNodeIdsList()) {
                                    try {
                                        executorService.submit(() -> {
                                            PutLocalResponse putLocalResponse = SDFSInternalServiceGrpc.newBlockingStub(getSharedChannel(nodeInfo.getNodeIp(),nodeInfo.getNodePort())).putLocal(
                                                    PutLocalRequest.newBuilder()
                                                            .setSdfsFileName(eachShardLocation.getSdfsFileName())
                                                            .setShardId(eachShardLocation.getShardId())
                                                            .setFileContent(ByteString.copyFrom(fileContent,
                                                                    eachShardLocation.getShardId() * Constants.SHARD_SIZE,
                                                                    Math.min(fileContent.length - eachShardLocation.getShardId() * Constants.SHARD_SIZE,Constants.SHARD_SIZE)))
                                                            .build());
                                                    if (putLocalResponse.getSuccess()) {
                                                        System.out.printf("Put %s:%d to %d successfully\n", eachShardLocation.getSdfsFileName(),eachShardLocation.getShardId(),
                                                                nodeInfo.getNodeIndex());
                                                    } else {
                                                        System.out.printf("Put %s:%d to %d failed\n", eachShardLocation.getSdfsFileName(),eachShardLocation.getShardId(),
                                                                nodeInfo.getNodeIndex());
                                                    }
//                                                    return putLocalResponse;
                                        });
                                    }
                                    catch (Exception ex) {
                                        System.out.printf("Put shard to %d failed\n", nodeInfo.getNodeIndex());
                                    }
                                }
                            }
                            executorService.shutdown();
                            executorService.awaitTermination(Long.MAX_VALUE, TimeUnit.NANOSECONDS);

                            //Step3: finish put: send the indices to leader
                            PutIndicesResponse putIndicesResponse = sdfsNodeServiceBlockingStub.finishPut(
                                    PutIndicesRequest.newBuilder()
                                            .setSdfsFileName(sdfsfilename)
                                            .addAllShards(initiatePutResponse.getShardsList())
                                            .build());
                            if (putIndicesResponse.getSuccess()) {
                                System.out.printf("Put %s finished successfully\n", sdfsfilename);
                            } else {
                                System.out.printf("Finish put %s failed\n", sdfsfilename);
                            }
                            long endTime = System.currentTimeMillis();
                            System.out.printf("Time used: %d\n", endTime - starTime);
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }

                    } else if (command.equals("get")) {
                        String sdfsfilename = cmd.getOptionValue("sdfsfilename");
                        String localfilename = cmd.getOptionValue("localfilename");
                        String ip = "127.0.0.1";
                        int port = Constants.PORT;
                        try {
                            long startTime = System.currentTimeMillis();
                            //Step1: initiate get: get the indices from leader
                            ManagedChannel managedChannel = getSharedChannel(ip, port);
                            SDFSNodeServiceGrpc.SDFSNodeServiceBlockingStub sdfsNodeServiceBlockingStub = SDFSNodeServiceGrpc.newBlockingStub(managedChannel);
                            InitiateGetResponse initiateGetResponse = sdfsNodeServiceBlockingStub.initiateGet(
                                    InitiateGetRequest.newBuilder().setSdfsFileName(sdfsfilename).build());
                            System.out.println(initiateGetResponse);
                            //Step2: Fetch the data
                            ExecutorService executorService = Executors.newFixedThreadPool(Constants.NUM_WORKERS);
                            ConcurrentHashMap<Integer, byte[]> shardIdToContent = new ConcurrentHashMap<>();
                            for (ShardReplicaLocations eachShardLocation : initiateGetResponse.getShardsList()) {
                                int shardId = eachShardLocation.getShardId();
                                int index = new Random().nextInt(eachShardLocation.getNodeIdsCount());
                                NodeId nodeId = eachShardLocation.getNodeIds(index);
                                executorService.submit(() -> {
                                    GetLocalResponse getLocalResponse = SDFSInternalServiceGrpc.newBlockingStub(getSharedChannel(nodeId.getNodeIp(),nodeId.getNodePort())).getLocal(
                                            GetLocalRequest.newBuilder()
                                                    .setShardId(shardId)
                                                    .setSdfsFileName(eachShardLocation.getSdfsFileName())
                                                    .build());
                                    if (getLocalResponse.getSuccess()) {
                                        shardIdToContent.put(shardId, getLocalResponse.getFileContent().toByteArray());
                                        System.out.printf("Get %s:%d from %d successfully\n", eachShardLocation.getSdfsFileName(), eachShardLocation.getShardId(),
                                                nodeId.getNodeIndex());
                                    } else {
                                        System.out.printf("Get %s:%d from %d failed\n", eachShardLocation.getSdfsFileName(), eachShardLocation.getShardId(),
                                                nodeId.getNodeIndex());
                                    }
                                });
                            }
                            executorService.shutdown();
                            executorService.awaitTermination(Long.MAX_VALUE, TimeUnit.NANOSECONDS);
                            //Step3: finish get: write file to the local repository
                            FinishGetResponse finishGetResponse = sdfsNodeServiceBlockingStub.finishGet(
                                    FinishGetRequest.newBuilder().setSdfsFileName(sdfsfilename).build());
                            if (finishGetResponse.getSuccess()) {
                                System.out.printf("Get %s finished successfully\n", sdfsfilename);
                            } else {
                                System.out.printf("Finish get %s failed\n", sdfsfilename);
                            }
                            //Step4: write file to the local repository
                            Utils.writeToFile(localfilename, shardIdToContent);
                            long endTime = System.currentTimeMillis();
                            System.out.printf("Time used: %d\n", endTime - startTime);
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }
                    } else if (command.equals("delete")) {
                        String sdfsfilename = cmd.getOptionValue("sdfsfilename");
                        String ip = "127.0.0.1";
                        int port = Constants.PORT;
                        try {
                            //Step1: initiate delete: get the indices from leader
                            ManagedChannel managedChannel = getSharedChannel(ip, port);
                            SDFSNodeServiceGrpc.SDFSNodeServiceBlockingStub sdfsNodeServiceBlockingStub = SDFSNodeServiceGrpc.newBlockingStub(managedChannel);
                            InitiateDeleteResponse initiateDeleteResponse = sdfsNodeServiceBlockingStub.initiateDelete(
                                    InitiateDeleteRequest.newBuilder()
                                            .setSdfsFileName(sdfsfilename)
                                            .build());
                            //Step2: Delete the file on the replicas
                            ExecutorService executorService = Executors.newFixedThreadPool(Constants.NUM_WORKERS);
                            for (ShardReplicaLocations eachShardLocation : initiateDeleteResponse.getShardsList()) {
                                for (NodeId nodeInfo : eachShardLocation.getNodeIdsList()) {
                                    try {
                                        executorService.submit(() -> {
                                            DeleteLocalResponse deleteLocalResponse = SDFSInternalServiceGrpc.newBlockingStub(getSharedChannel(nodeInfo.getNodeIp(),nodeInfo.getNodePort())).deleteLocal(
                                                    DeleteLocalRequest.newBuilder()
                                                            .setSdfsFileName(eachShardLocation.getSdfsFileName())
                                                            .setShardId(eachShardLocation.getShardId())
                                                            .build());
                                            if (deleteLocalResponse.getSuccess()) {
                                                System.out.printf("Delete %s:%d from %d successfully\n", eachShardLocation.getSdfsFileName(),eachShardLocation.getShardId(),
                                                        nodeInfo.getNodeIndex());
                                            } else {
                                                System.out.printf("Delete %s:%d from %d failed\n", eachShardLocation.getSdfsFileName(),eachShardLocation.getShardId(),
                                                        nodeInfo.getNodeIndex());
                                            }
                                        });
                                    }
                                    catch (Exception ex) {
                                        System.out.printf("Delete shard to %d failed\n", nodeInfo.getNodeIndex());
                                    }
                                }
                            }
                            executorService.shutdown();
                            executorService.awaitTermination(Long.MAX_VALUE, TimeUnit.NANOSECONDS);
                            //Step3: finish delete: send the indices to leader
                            FinishDeleteResponse finishDeleteResponse = sdfsNodeServiceBlockingStub.finishDelete(
                                    FinishDeleteRequest.newBuilder()
                                            .setSdfsFileName(sdfsfilename)
                                            .build());
                            if (finishDeleteResponse.getSuccess()) {
                                System.out.printf("Delete %s finished successfully\n", sdfsfilename);
                            } else {
                                System.out.printf("Finish delete %s failed\n", sdfsfilename);
                            }
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }

                    } else if (command.equals("ls")) {
                        //ls sdfsfilename: list all machine (VM) addresses where this file is currently being stored
                        String sdfsfilename = cmd.getOptionValue("sdfsfilename");
                        String ip = "127.0.0.1";
                        int port = Constants.PORT;
                        try {
                            //Step1: initiate get: get the indices from leader
                            ManagedChannel managedChannel = ManagedChannelBuilder
                                    .forAddress(ip, port)
                                    .usePlaintext()
                                    .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                                    .build();
                            SDFSNodeServiceGrpc.SDFSNodeServiceBlockingStub sdfsNodeServiceBlockingStub = SDFSNodeServiceGrpc.newBlockingStub(managedChannel);
                            InitiateGetResponse initiateGetResponse = sdfsNodeServiceBlockingStub.initiateGet(
                                    InitiateGetRequest.newBuilder().setSdfsFileName(sdfsfilename).build());
                            managedChannel.shutdown();
                            //Step2: print the result
                            /*
                            * Filename:
                            * Shard 1: 1 2 3
                            * Shard 2: 2 3 4
                            *
                            * */
                            System.out.printf("The file %s is stored at the following locations:\n", sdfsfilename);
                            for (int i=0; i<initiateGetResponse.getShardsCount();i++){
                                ShardReplicaLocations shardReplicaLocations = initiateGetResponse.getShards(i);
                                System.out.printf("Shard %d: ", shardReplicaLocations.getShardId());
                                for (NodeId nodeId : shardReplicaLocations.getNodeIdsList()) {
                                    System.out.printf("%d ", nodeId.getNodeIndex());
                                }
                                System.out.println();
                            }
                        }
                        catch (Exception ex) {
                            System.out.println("ls failed");
                        }
                    } else if (command.equals("store")) {
                        //store: At any machine, list all files currently being stored at this machine.
                        String ip = "127.0.0.1";
                        int port = Constants.PORT;
                        try {
                            //Step1: initiate get: get the indices from leader
                            ManagedChannel managedChannel = ManagedChannelBuilder
                                    .forAddress(ip, port)
                                    .usePlaintext()
                                    .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                                    .build();
                            SDFSInternalServiceGrpc.SDFSInternalServiceBlockingStub sdfsInternalServiceBlockingStub =
                                    SDFSInternalServiceGrpc.newBlockingStub(managedChannel);
                            GetShardInfoResponse getShardInfoResponse = sdfsInternalServiceBlockingStub.getShardInfo(Empty.newBuilder().build());
                            //Step2: print the result
                            /*
                            *
                            * Machine has the following files:
                            * Filename1: shard1
                            * Filename1: shard2
                            * Filename2: shard1
                            *
                            * */
                            System.out.println("Machine has the following files:");
                            for (int i=0;i<getShardInfoResponse.getShardInfosCount();i++){
                                ShardInfo shardInfo = getShardInfoResponse.getShardInfos(i);
                                System.out.printf("%s: %d\n", shardInfo.getSdfsFileName(), shardInfo.getShardId());
                            }
                        }
                        catch (Exception ex) {
                            System.out.println("store failed");
                        }
                    }
                    else if (command.equals("multiread")) {
                        //need config file to get the info of the cluster
                        List<Integer> nodes = new ArrayList<>();
                        for (int i= 0; i<cmd.getOptionValues("multiread").length; i++) {
                            nodes.add(Integer.parseInt(cmd.getOptionValues("multiread")[i]));
                        }
                        String sdfsfilename = cmd.getOptionValue("sdfsfilename");
                        String localfilename = cmd.getOptionValue("localfilename");
                        Map<Integer, NodeInfo> list = Utils.readNodeConfig(args[0]);
                        try{
                            for (int i = 0; i < nodes.size(); i++) {
                                String s = String.format("ssh xz112@%s " +
                                        "&& cd ../target " +
                                        "&& java -cp sdfs-1.0-SNAPSHOT-shaded.jar edu.illinois.cs425.Command " +
                                        "-put -sdfsfilename %s -localfilename %s", list.get(nodes.get(i)).getIp(), sdfsfilename, localfilename);
                                Process p = Runtime.getRuntime().exec(s);
                                PrintStream out = new PrintStream(p.getOutputStream());
                                out.println("Finished!");
                            }
                        }catch (Exception e){
                            e.printStackTrace();
                        }
                    }
                    else {

                    }
                }
            }
        } catch (ParseException e) {
            throw new RuntimeException(e);
        }
        for (ManagedChannel channel : sharedChannel.values()) {
            channel.shutdown();
        }
    }

    private static ManagedChannel getSharedChannel(String ip, int port) {
        synchronized (sharedChannel) {
            if (!sharedChannel.containsKey(ip)) {
                ManagedChannel managedChannel = ManagedChannelBuilder
                        .forAddress(ip,port)
                        .usePlaintext()
                        .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                        .maxInboundMessageSize(Integer.MAX_VALUE)
                        .build();
                sharedChannel.put(ip, managedChannel);
            }
        }
        return sharedChannel.get(ip);
    }
}
