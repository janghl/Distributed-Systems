package edu.illinois.cs425.utils;

import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;

public final class Constants {
    public static final float f = 0.5f;
    public static final int TIME_CHECK = 500; //ms
    public static final int TIME_FAIL = 2000; //ms
    public static final int TIME_CLEANUP = 3000; //ms
    public static final int TIME_GOSSIP = 500; //ms
    public static final int TIME_SUSPICION = 2000;
    public static final String INTRODUCER_IP = "fa23-cs425-5201.cs.illinois.edu"; //I think we should be able to config introducer
    public static final int INTRODUCER_INDEX = 1;
    public static final int PORT = 8000;
    public static final int PORT_UDP = 8001;
    public static final float DROP_RATE = 0f;
    public static AtomicBoolean SUSPICION = new AtomicBoolean(false);

    //=======================leader election=======================
    //we have initial leader
    public static int LEADER_INDEX = 2;
    public static String LEADER_IP = "fa23-cs425-5202.cs.illinois.edu";
    public static final int CLUSTER_SIZE = 5; //todo: need to check whether this is needed.
    public static final int LEADER_TIMEOUT = 1000; //ms

    //=======================sdfs=======================
    public static final String SDFS_ROOT = "mp3/sdfs/";
    public static final String LOCAL_ROOT = "mp3/local/";
    public static final int SHARD_SIZE =  64 * 1000 * 1000;
    public static final int NUM_REPLICAS = 4;
    public static final Random rand = new Random();
    public static final int NUM_WORKERS = 4;
}