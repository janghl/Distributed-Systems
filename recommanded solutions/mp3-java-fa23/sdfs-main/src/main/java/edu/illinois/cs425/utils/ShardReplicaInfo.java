package edu.illinois.cs425.utils;

import edu.illinois.cs425.grpcServices.ShardReplicaLocations;

import java.util.List;
import java.util.stream.Collectors;

public class ShardReplicaInfo {
    String fileName;
    int shardId;
    List<NodeInfo> replicas;

    public ShardReplicaInfo(String fileName, int shardId, List<NodeInfo> replicas) {
        this.fileName = fileName;
        this.shardId = shardId;
        this.replicas = replicas;
    }

    public ShardReplicaInfo(ShardReplicaLocations shardReplicaLocations) {
        this.fileName = shardReplicaLocations.getSdfsFileName();
        this.shardId = shardReplicaLocations.getShardId();
        this.replicas = shardReplicaLocations.getNodeIdsList().stream().map(NodeInfo::new).collect(Collectors.toList());
    }

    public ShardReplicaLocations toMessage() {
        ShardReplicaLocations shardReplicaLocations = ShardReplicaLocations.newBuilder()
                .setShardId(shardId)
                .addAllNodeIds(replicas.stream().map(NodeInfo::toMessage).collect(Collectors.toList()))
                .setSdfsFileName(fileName)
                .build();
        return shardReplicaLocations;
    }

    public void addReplica(NodeInfo replica) {
        replicas.add(replica);
    }

    public String getFileName() {
        return fileName;
    }

    public int getShardId() {
        return shardId;
    }

    public List<NodeInfo> getReplicas() {
        return replicas;
    }
}
