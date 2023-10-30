package edu.illinois.cs425.utils;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.Map;

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
                    (Integer) jsonObject.get("port")
            );
            nodeDetails.put(nodeId, nodeInfo);
        }
        return nodeDetails;
    }
}
