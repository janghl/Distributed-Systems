package edu.illinois.cs425.service;

import com.google.protobuf.ByteString;
import edu.illinois.cs425.grpcServices.*;
import edu.illinois.cs425.utils.Constants;
import edu.illinois.cs425.utils.NodeInfo;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import io.grpc.stub.StreamObserver;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.TimeUnit;

public class SDFSNodeService extends SDFSNodeServiceGrpc.SDFSNodeServiceImplBase {
    private static ConcurrentMap<String, ManagedChannel> sharedChannels ;

    public SDFSNodeService(ConcurrentHashMap<String, ManagedChannel> sharedChannels) {
        this.sharedChannels = sharedChannels;
    }

    @Override
    public void initiatePut(InitiatePutRequest request, StreamObserver<InitiatePutResponse> responseObserver) {
        HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(getSharedChannels(Constants.LEADER_IP, Constants.PORT));
        InitiatePutResponseLeader initiatePutResponseLeader = heartbeatServerBlockingStub.initiatePutLeader(
                InitiatePutRequestLeader.newBuilder()
                        .setSdfsFileName(request.getSdfsFileName())
                        .setFileSize(request.getFileSize())
                        .build());
        int newNumShards = (int) Math.ceil(request.getFileSize() * 1.0 / Constants.SHARD_SIZE);
        responseObserver.onNext(InitiatePutResponse.newBuilder().addAllShards(initiatePutResponseLeader.getShardsList().subList(0, newNumShards)).build());
        responseObserver.onCompleted();
        int oldNumShards = initiatePutResponseLeader.getShardsCount();
        for (int i = newNumShards; i < oldNumShards; i++) {
            for (NodeId nodeId : initiatePutResponseLeader.getShards(i).getNodeIdsList()) {
                assert SDFSInternalServiceGrpc.newBlockingStub(getSharedChannels(nodeId.getNodeIp(),nodeId.getNodePort())).deleteLocal(
                        DeleteLocalRequest.newBuilder().setSdfsFileName(request.getSdfsFileName()).setShardId(i).build()).getSuccess();
            }
        }
    }

    public void finishPut(PutIndicesRequest request, StreamObserver<PutIndicesResponse> responseObserver) {
        HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(getSharedChannels(Constants.LEADER_IP, Constants.PORT));
        PutIndicesResponseLeader putIndicesResponseLeader = heartbeatServerBlockingStub.finishPutLeader(
                PutIndicesRequestLeader.newBuilder()
                        .setSdfsFileName(request.getSdfsFileName())
                        .addAllShards(request.getShardsList()).build());
        responseObserver.onNext(PutIndicesResponse.newBuilder().setSuccess(putIndicesResponseLeader.getSuccess()).build());
        responseObserver.onCompleted();
    }
    @Override
    public void initiateGet(InitiateGetRequest request, StreamObserver<InitiateGetResponse> responseObserver) {
        HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(getSharedChannels(Constants.LEADER_IP, Constants.PORT));
        InitiateGetResponseLeader initiateGetResponseLeader = heartbeatServerBlockingStub.initiateGetLeader(
                InitiateGetRequestLeader.newBuilder().setSdfsFileName(request.getSdfsFileName()).build());
        responseObserver.onNext(InitiateGetResponse.newBuilder().addAllShards(initiateGetResponseLeader.getShardsList()).build());
        responseObserver.onCompleted();
    }

    @Override
    public void finishGet(FinishGetRequest request, StreamObserver<FinishGetResponse> responseObserver) {
        HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(getSharedChannels(Constants.LEADER_IP, Constants.PORT));
        FinishGetResponseLeader finishGetResponseLeader = heartbeatServerBlockingStub.finishGetLeader(
                FinishGetRequestLeader.newBuilder().setSdfsFileName(request.getSdfsFileName()).build());
        responseObserver.onNext(FinishGetResponse.newBuilder().setSuccess(finishGetResponseLeader.getSuccess()).build());
        responseObserver.onCompleted();
    }

    @Override
    public void initiateDelete(InitiateDeleteRequest request, StreamObserver<InitiateDeleteResponse> responseObserver) {
        HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(getSharedChannels(Constants.LEADER_IP, Constants.PORT));
        InitiateDeleteResponseLeader initiateDeleteResponseLeader = heartbeatServerBlockingStub.initiateDeleteLeader(
                InitiateDeleteRequestLeader.newBuilder().setSdfsFileName(request.getSdfsFileName()).build());
        responseObserver.onNext(InitiateDeleteResponse.newBuilder()
                .addAllShards(initiateDeleteResponseLeader.getShardsList()).build());
        responseObserver.onCompleted();
    }

    public void finishDelete(FinishDeleteRequest request, StreamObserver<FinishDeleteResponse> responseObserver) {
        HeartbeatServerGrpc.HeartbeatServerBlockingStub heartbeatServerBlockingStub = HeartbeatServerGrpc.newBlockingStub(getSharedChannels(Constants.LEADER_IP, Constants.PORT));
        FinishDeleteResponseLeader finishDeleteResponseLeader = heartbeatServerBlockingStub.finishDeleteLeader(
                FinishDeleteRequestLeader.newBuilder().setSdfsFileName(request.getSdfsFileName()).build());
        responseObserver.onNext(FinishDeleteResponse.newBuilder().setSuccess(finishDeleteResponseLeader.getSuccess()).build());
        responseObserver.onCompleted();
    }

    @Override
    public void replicateShard(ReplicateShardRequest request, StreamObserver<ReplicateShardResponse> responseObserver) {
        final Path localPath = Paths.get(Constants.SDFS_ROOT).resolve(request.getSdfsFileName() + "_" + request.getShardId());
        PutLocalRequest.Builder builder = PutLocalRequest.newBuilder();
        builder.setShardId(request.getShardId());
        builder.setSdfsFileName(request.getSdfsFileName());
        try {
            byte[] fileBytes = Files.readAllBytes(localPath);
            builder.setFileContent(ByteString.copyFrom(fileBytes));
        } catch (IOException e) {
            System.out.printf("local read (for replication) for %s:%d failed!\n", request.getSdfsFileName(), request.getShardId());
        }
        PutLocalResponse putLocalResponse = SDFSInternalServiceGrpc.newBlockingStub(getSharedChannels(request.getNewReplica().getNodeIp(),request.getNewReplica().getNodePort())).putLocal(builder.build());
        responseObserver.onNext(ReplicateShardResponse.newBuilder().setSuccess(putLocalResponse.getSuccess()).build());
        responseObserver.onCompleted();
    }

    private static ManagedChannel getSharedChannels(String ip, int port) {
        synchronized (sharedChannels) {
            if (!sharedChannels.containsKey(ip)) {
                ManagedChannel managedChannel = ManagedChannelBuilder
                        .forAddress(ip, port)
                        .usePlaintext()
                        .keepAliveTime(Long.MAX_VALUE, TimeUnit.DAYS)
                        .build();
                sharedChannels.put(ip, managedChannel);
            }
        }
        return sharedChannels.get(ip);
    }
}
