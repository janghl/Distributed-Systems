package edu.illinois.cs425.utils;

import org.json.JSONArray;
import org.json.JSONObject;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public final class Utils {
    public static Map<Integer, NodeInfo> readNodeConfig(String nodesConfigFilePath) {
        //Read the node config file
        Map<Integer, NodeInfo> nodeDetails = new HashMap<>();
        JSONArray jsonArray = null;
        try {
            jsonArray = new JSONArray(Files.readString(Paths.get(nodesConfigFilePath)));
        } catch (IOException e) {
            e.printStackTrace();
        }
        for(Object obj : jsonArray) {
            JSONObject jsonObject = (JSONObject) obj;
            int nodeId = jsonObject.getInt("id");
            NodeInfo nodeInfo = new NodeInfo(
                    nodeId,
                    (String) jsonObject.get("ip"),
                    Constants.PORT
            );
            nodeDetails.put(nodeId, nodeInfo);
        }
        return nodeDetails;
    }

    public static void writeToFile(String filePath, ConcurrentHashMap<Integer, byte[]> content) {
        try {
            Path localRoot = Path.of(Constants.LOCAL_ROOT);
            if (!Files.exists(localRoot)) {
                Files.createDirectories(localRoot);
            }
            final Path absoluteFilePath = localRoot.resolve(filePath);
            if (Files.exists(absoluteFilePath)) {
                Files.delete(absoluteFilePath);
            }
            Files.createFile(absoluteFilePath);
            for (int index = 0; index < content.keySet().size();index++) {
                Files.write(absoluteFilePath, content.get(index), StandardOpenOption.APPEND);
            }
        } catch (Exception e) {
            System.out.println("local write failed!");
        }
    }

    public static byte[] readFile(String filePath) {
        String absoluteFilePath = Constants.LOCAL_ROOT + filePath;
        try {
            return Files.readAllBytes(Paths.get(absoluteFilePath));
        } catch (IOException e) {
            System.out.println("local read failed!");
        }
        return null;
    }
}
