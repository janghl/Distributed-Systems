package edu.illinois.cs425.utils;

public class MembershipInfo {
    private int heartbeatCounter;

    private NodeStatus nodeStatus;

    private int nodeTimeout;

    private int incarnation;

    public MembershipInfo(int heartbeatCounter, NodeStatus nodeStatus, int nodeTimeout, int incarnation) {
        this.heartbeatCounter = heartbeatCounter;
        this.nodeStatus = nodeStatus;
        this.nodeTimeout = nodeTimeout;
        this.incarnation = incarnation;
    }

    public int getHeartbeatCounter() {
        return heartbeatCounter;
    }

    public void setHeartbeatCounter(int heartbeatCounter) {
        this.heartbeatCounter = heartbeatCounter;
    }

    public NodeStatus getNodeStatus() {
        return nodeStatus;
    }

    public void setNodeStatus(NodeStatus nodeStatus) {
        this.nodeStatus = nodeStatus;
    }

    public int getNodeTimeout() {
        return nodeTimeout;
    }

    public void setNodeTimeout(int nodeTimeout) {
        this.nodeTimeout = nodeTimeout;
    }

    public void incrementHeartbeatCounter() {
        this.heartbeatCounter++;
    }

    public void incrementNodeTimeout(int time) {
        this.nodeTimeout += time;
    }

    public int getIncarnation() {
        return incarnation;
    }

    public void setIncarnation(int incarnation) {
        this.incarnation = incarnation;
    }

    public void incrementIncarnation() {
        this.incarnation++;
    }
}
