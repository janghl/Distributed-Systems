# **Dependency**:

- JAVA 11
- gRPC v1.58.0


# **Building Jar**
```
mvn clean package
```
# **Running**

**Run the server process** **on each virtual machine**

```
java -cp [JAR_PATH] edu.illinois.cs425.server.NodeServer [NODE_ID]
```

for example:

```
java -cp log-querier-1.0-SNAPSHOT-shaded.jar edu.illinois.cs425.server.NodeServer 1
java -cp log-querier-1.0-SNAPSHOT-shaded.jar edu.illinois.cs425.server.NodeServer 2
```

# **Running Commands**

### **Configuration File:**

A config.json file should specify the nodes which need mode switching (typically the entire cluster).

```
{
        "id": 1,
        "ip": "fa23-cs425-5201.cs.illinois.edu",
        "port": 8000
    },
    {
        "id": 2,
        "ip": "fa23-cs425-5202.cs.illinois.edu",
        "port": 8000
    },
 }
```

**To put local file into SDFS**

```
java -cp sdfs-1.0-SNAPSHOT-shaded.jar edu.illinois.cs425.Command -put -sdfsfilename [sdfsfilename] -localfilename [localfilename]
```


**To get a remote file into local fs**

```
 java -cp sdfs-1.0-SNAPSHOT-shaded.jar edu.illinois.cs425.Command -get -sdfsfilename [sdfsfilename] -localfilename [localfilename]
```