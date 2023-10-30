package edu.illinois.cs425.utils;

import java.util.concurrent.atomic.AtomicBoolean;

public final class Constants {
    //the fraction of nodes that are chosen randomly each round
    //should be >= 0.5, if in 2-node system, there will be no gossiping
    //TODO: decide if f this is proper
    //TODO: decide the proper value for different variables
    //TODO: actually a question: how can introducer join the network again? I suppose cannot.
    public static final float f = 0.5f;
    public static final int TIME_CHECK = 2000; //ms
    public static final int TIME_FAIL = 3000; //ms
    public static final int TIME_CLEANUP = 2000; //ms
    public static final int TIME_GOSSIP = 500; //ms
    public static final int TIME_SUSPICION = 2000;
    public static final String INTRODUCER_IP = "fa23-cs425-5201.cs.illinois.edu"; //I think we should be able to config introducer
    public static final int INTRODUCER_INDEX = 1;
    public static final int PORT = 8000;
    public static final int PORT_UDP = 8001;
    public static final float DROP_RATE = 0f;
    public static AtomicBoolean SUSPICION = new AtomicBoolean(false);

    // is there a case that the heartbeat is larger but the incarnation is smaller?
    // before the node is marked as failed, we may also send the heartbeat to the node, which will report the failure
    // there's some error, when the node is not finish introduce
}