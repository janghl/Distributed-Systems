package edu.illinois.cs425;

import com.google.protobuf.Empty;
import edu.illinois.cs425.grpcServices.*;
import edu.illinois.cs425.grpcServices.HeartbeatServerGrpc;
import edu.illinois.cs425.utils.Constants;
import edu.illinois.cs425.utils.NodeInfo;
import edu.illinois.cs425.utils.NodeStatus;
import edu.illinois.cs425.utils.Utils;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import org.apache.commons.cli.*;

import java.util.Map;
import java.util.concurrent.TimeUnit;

public class Command {
    public static void main(String[] args) {
        Options options = new Options();
        // Help and Version
        options.addOption("h", "help", false, "Output a help message and exit.");
        options.addOption("V", "version", false, "Output the version number and exit.");

        options.addOption("s", "suspicion", false, "Switch to suspicion mode");
        options.addOption("ns", "no-suspicion", false, "Switch to normal mode.");
        options.addOption("l", "leave", false, "Leave the cluster.");
        options.addOption("ml", "membership-list", false, "Get membership list.");
        options.addOption("n", "node-id", false, "Get node id.");
        options.addOption("ls", "list-self", false, "List self id.");
        options.addOption("ip", "ip", true, "Get node id.");
        options.addOption("p", "port", true, "Get node id.");

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
                        String ip = cmd.getOptionValue("ip");
                        int port = Integer.parseInt(cmd.getOptionValue("port"));
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
                    }
                }
            }
        } catch (ParseException e) {
            throw new RuntimeException(e);
        }
    }
}
