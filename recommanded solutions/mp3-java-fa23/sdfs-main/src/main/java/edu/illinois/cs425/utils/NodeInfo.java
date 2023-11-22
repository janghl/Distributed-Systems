package edu.illinois.cs425.utils;

import edu.illinois.cs425.grpcServices.NodeId;

import java.util.Objects;

public class NodeInfo {
    private int id;
    private String ip;
    private int port;
    private long timestamp;

    public NodeInfo(int id, String ip, int port) {
        this.id = id;
        this.ip = ip;
        this.port = port;
    }

    public NodeInfo(NodeId nodeId) {
        this.id = nodeId.getNodeIndex();
        this.ip = nodeId.getNodeIp();
        this.port = nodeId.getNodePort();
        this.timestamp = nodeId.getNodeTimestamp();
    }

    public NodeId toMessage() {
        return NodeId.newBuilder()
                .setNodeIndex(this.id)
                .setNodeIp(this.ip)
                .setNodePort(this.port)
                .setNodeTimestamp(this.timestamp)
                .build();
    }

    public int getId() {
        return id;
    }

    public String getIp() {
        return ip;
    }

    public int getPort() {
        return port;
    }

    public void setTimestamp(long timestamp) {
        this.timestamp = timestamp;
    }

    public long getTimestamp() {
        return timestamp;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        NodeInfo nodeInfo = (NodeInfo) o;
        return port == nodeInfo.port && Objects.equals(ip, nodeInfo.ip) && timestamp == nodeInfo.timestamp;
    }

    @Override
    public int hashCode() {
        return Objects.hash(ip, port, timestamp);
    }
}
