package edu.illinois.cs425.service;

import com.google.protobuf.ByteString;
import com.google.protobuf.Empty;
import edu.illinois.cs425.grpcServices.*;
import edu.illinois.cs425.utils.Constants;
import io.grpc.stub.StreamObserver;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class SDFSInternalService extends SDFSInternalServiceGrpc.SDFSInternalServiceImplBase {

    public SDFSInternalService() {
        try {
            Path rootPath = Paths.get(Constants.SDFS_ROOT);
            File[] files = rootPath.toFile().listFiles();
            for (File file : files) {
                Files.deleteIfExists(file.toPath());
            }
            Files.deleteIfExists(Paths.get(Constants.SDFS_ROOT));
            Files.createDirectories(Paths.get(Constants.SDFS_ROOT));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    @Override
    public void putLocal(PutLocalRequest request, StreamObserver<PutLocalResponse> responseObserver) {
        final Path localPath = Paths.get(Constants.SDFS_ROOT).resolve(getShardFileName(request.getSdfsFileName(),request.getShardId()));
        PutLocalResponse.Builder putLocalResponseBuilder = PutLocalResponse.newBuilder();
        try {
            Files.write(localPath, request.getFileContent().toByteArray());
            putLocalResponseBuilder.setSuccess(true);
        } catch (IOException e) {
            System.out.printf("local write for %s:%d failed!\n", request.getSdfsFileName(), request.getShardId());
            putLocalResponseBuilder.setSuccess(false);
        }
        responseObserver.onNext(putLocalResponseBuilder.build());
        responseObserver.onCompleted();
    }

    @Override
    public void getLocal(GetLocalRequest request, StreamObserver<GetLocalResponse> responseObserver) {
        final Path localPath = Paths.get(Constants.SDFS_ROOT).resolve(request.getSdfsFileName() + "_" + request.getShardId());
        GetLocalResponse.Builder getLocalResponseBuilder = GetLocalResponse.newBuilder();
        try {
            byte[] fileBytes = Files.readAllBytes(localPath);
            getLocalResponseBuilder.setFileContent(ByteString.copyFrom(fileBytes));
            getLocalResponseBuilder.setSuccess(true);
        } catch (IOException e) {
            System.out.printf("local read for %s:%d failed!\n", request.getSdfsFileName(), request.getShardId());
            getLocalResponseBuilder.setSuccess(false);
        }
        responseObserver.onNext(getLocalResponseBuilder.build());
        responseObserver.onCompleted();
    }

    @Override
    public void deleteLocal(DeleteLocalRequest request, StreamObserver<DeleteLocalResponse> responseObserver) {
        final Path localPath = Paths.get(Constants.SDFS_ROOT).resolve(request.getSdfsFileName() + "_" + request.getShardId());
        final File localFile = new File(localPath.toUri());
        final boolean success = localFile.delete();
        DeleteLocalResponse deleteLocalResponse = DeleteLocalResponse.newBuilder().setSuccess(success).build();
        responseObserver.onNext(deleteLocalResponse);
        responseObserver.onCompleted();
    }

    public String getShardFileName(String sdfsFileName, int shardId) {
        return sdfsFileName + "_" + shardId;
    }

    @Override
    public void getShardInfo(Empty empty, StreamObserver<GetShardInfoResponse> responseObserver) {
        final Path path = Paths.get(Constants.SDFS_ROOT);
        final File[] files = path.toFile().listFiles();
        GetShardInfoResponse.Builder getShardInfoResponse = GetShardInfoResponse.newBuilder();
        for (File file : files) {
            String fileName = file.getName();
            String sdfFileName = fileName.substring(0, fileName.lastIndexOf("_"));
            int shardId = Integer.parseInt(fileName.substring(fileName.lastIndexOf("_") + 1));
            getShardInfoResponse.addShardInfos(ShardInfo.newBuilder().setSdfsFileName(sdfFileName).setShardId(shardId).build());
        }
        responseObserver.onNext(getShardInfoResponse.build());
        responseObserver.onCompleted();
    }
}

