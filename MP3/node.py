import socket
import time
import select
import threading
import json
import os
import sys
import random
import datetime
import logging
import argparse
import queue
import traceback
# Define a list of host names that represent nodes in the distributed system.
# These host names are associated with specific machines in the network.
# The 'Introducer' variable points to a specific host in the system that may serve as an introducer node.

HOST_NAME_LIST = [
    'fa23-cs425-9501.cs.illinois.edu',
    'fa23-cs425-9502.cs.illinois.edu',
    'fa23-cs425-9503.cs.illinois.edu',
    'fa23-cs425-9504.cs.illinois.edu',
    'fa23-cs425-9505.cs.illinois.edu',
    'fa23-cs425-9506.cs.illinois.edu',
    'fa23-cs425-9507.cs.illinois.edu',
    'fa23-cs425-9508.cs.illinois.edu',
    'fa23-cs425-9509.cs.illinois.edu',
    'fa23-cs425-9510.cs.illinois.edu',
]
# 'Introducor' specifies the introducer node's hostname, which plays a crucial role in system coordination.
Introducor = 'fa23-cs425-9502.cs.illinois.edu'
# 'DEFAULT_PORT_NUM' defines the default port number used for communication within the system.
DEFAULT_PORT_NUM = 12360
SERVER_PORT = 10000
LEADER_PORT = 8080
LIST_PORT = 8020
# Configure logging for the script. This sets up a logging system that records debug information
# to the 'output.log' file, including timestamps and log levels.
logging.basicConfig(level=logging.DEBUG,
                    filename='output.log',
                    datefmt='%Y/%m/%d %H:%M:%S',
                    format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# The `Server` class represents a node in a distributed system.
class Server:
    def __init__(self,args):
        # Initialize the server with various attributes.
        if os.path.exists("leader_list.json"):
            with open("leader_list.json", 'r') as f:
                self.leader_list = json.load(f)
        self.ip = socket.gethostname()
        self.read_queue = queue.Queue()
        self.recently_read = True
        self.write_queue = queue.Queue()
        self.write_queue_lock = threading.Lock()
        self.port = DEFAULT_PORT_NUM
        self.heartbeat = 0
        self.timejoin = int(time.time())
        self.id = f"{self.ip}:{self.port}:{self.timejoin}"
        self.leader = 'fa23-cs425-9501.cs.illinois.edu' # init leader to VM 2
        self.replica_leaders = ['fa23-cs425-9501.cs.illnois.edu', 'fa23-cs425-9503.cs.illnois.edu', 'fa23-cs425-9504.cs.illnois.edu']
        self.leader_list = {}

        self.id_num = int(self.ip[13:15])
        self.addr = (self.ip, self.port)
        self.MembershipList = {
                f"{ip}:{port}:{self.timejoin}": {
                "id": f"{ip}:{port}:{self.timejoin}",
                "addr": (ip, port),
                "heartbeat": 0,
                "incarnation":0,
                "status": "Alive",
                "time": time.time(),
                "id_number": int(ip[13:15]) 
            }
            for ip, port in [(IP, DEFAULT_PORT_NUM) for IP in [self.ip, Introducor]]
        }
        # List to track failed members.
        self.failMemberList = {}
        # Thresholds for various time-based criteria.
        self.failure_time_threshold = 7
        self.cleanup_time_threshold = 7
        self.suspect_time_threshold = 7
        self.protocol_period = args.protocol_period
        # Number of times to send messages.
        self.n_send = 3
        self.status = "Alive"
        # Probability of dropping a message.
        self.drop_rate = args.drop_rate
        # Incarnation number for handling suspicion.
        self.incarnation = 0
        # Thread-safe lock for synchronization.
        self.rlock = threading.RLock()
        # Flag to enable or disable message sending for leaving group and enable and disable suspicion mechanisism
        self.enable_sending = True
        self.gossipS = False

    
    def printID(self):
        # Method to print the unique ID of the server.
        with self.rlock:
            print(self.id)

    def updateMembershipList(self, membershipList):
        # Method to update the membership list of the server with received information.
        with self.rlock:
            # Iterate through the received membership list.
            for member_id, member_info in membershipList.items():
                if member_info['heartbeat'] == 0:
                    # Skip members with heartbeat equal to 0, to clear out the initial introducor with 0 heartbeat.
                    continue
                if member_id in self.failMemberList:
                    # Skip members that are already in the failed members list.
                    continue

                member_info.setdefault("status", "Alive")
                member_info.setdefault("incarnation", 0)
                #if the server receive the suspect message about itself, overwrite the message with great incarnation number:
                if member_id == self.id:
                    if member_info["status"] == "Suspect":
                         if self.incarnation < member_info["incarnation"]:
                            self.incarnation = member_info["incarnation"] + 1
                # Check if the member is already in the MembershipList
                if member_id in self.MembershipList:
                    current_heartbeat = self.MembershipList[member_id]["heartbeat"]
                    # Incarnation overwrite heartbeat
                    if member_info["incarnation"] > self.MembershipList[member_id]["incarnation"]:
                        self.MembershipList[member_id] = member_info
                        self.MembershipList[member_id]["time"] = time.time()
                        #if suspect print out
                        if self.MembershipList[member_id]["status"] == "Suspect":
                            logger.info("[SUS]    - {}".format(member_id))
                            log_message = f"Incaroverwrite: ID: {member_id}, Status: {self.MembershipList[member_id]['status']}, Time: {self.MembershipList[member_id]['time']}\n"
                            print(log_message)
                    # Update only if the received heartbeat is greater and both at the same incarnation
                    elif member_info["heartbeat"] > current_heartbeat and member_info["incarnation"] == self.MembershipList[member_id]["incarnation"]:
                        self.MembershipList[member_id] = member_info
                        self.MembershipList[member_id]["time"] = time.time()
                else:
                    # If the member is not in the MembershipList, add it
                    self.MembershipList[member_id] = member_info
                    self.MembershipList[member_id]["time"] = time.time()
                    # If suspect print out 
                    if self.MembershipList[member_id]["status"] == "Suspect":
                        logger.info("[SUS]    - {}".format(member_id))
                        log_message = f"Newmem        : ID: {member_id}, Status: {self.MembershipList[member_id]['status']}, Time: {self.MembershipList[member_id]['time']}\n"
                        print(log_message)
                    logger.info("[JOIN]   - {}".format(member_id))

    def detectSuspectAndFailMember(self):
        # Method to detect and handle suspected and failed members in the membership list for the gossip S protocol.
        with self.rlock:
            now = int(time.time())
            # Calculate the threshold time
            failure_threshold_time = now - self.failure_time_threshold
            suspect_threshold_time = now - self.suspect_time_threshold
            # Collect members to remove
            suspect_members_detected = [member_id for member_id, member_info in self.MembershipList.items() if member_info['time'] < failure_threshold_time and member_info["status"] != "Suspect"]
            for member_id in suspect_members_detected:
                self.MembershipList[member_id]["status"] = "Suspect"
                self.MembershipList[member_id]["incarnation"] += 1
                self.MembershipList[member_id]["time"] = now
                logger.info("[SUS]    - {}".format(member_id))
                log_message = f"Detected      : ID: {member_id}, Status: {self.MembershipList[member_id]['status']}, Time: {self.MembershipList[member_id]['time']}\n"
                print(log_message)
            fail_members_detected = [member_id for member_id, member_info in self.MembershipList.items() if member_info['time'] < suspect_threshold_time and member_id not in self.failMemberList and member_info['status'] == "Suspect"]
            for member_id in fail_members_detected:
                
                self.failMemberList[member_id] = now
                del self.MembershipList[member_id]
                logger.info("[DELETE] - {}".format(member_id))
                # TODO: if this member_id was the leader -> elect_leader() -> either declare itself leader or intitiate an election run
                self.elect_leader()

    def detectFailMember(self):
        # Method to detect and handle failed members in the membership list for the gossip protocol.
        with self.rlock:
            now = int(time.time())
            # Calculate the threshold time
            threshold_time = now - self.failure_time_threshold
            # Collect members to remove
            fail_members_detected = [member_id for member_id, member_info in self.MembershipList.items() if member_info['time'] < threshold_time and member_id not in self.failMemberList]
            for member_id in fail_members_detected:
                self.failMemberList[member_id] = now
                addr = self.MembershipList[member_id]['addr'][0]
                del self.MembershipList[member_id]
                logger.info("[DELETE] - {}".format(member_id))
                # TODO: if this member_id was the leader -> elect_leader() -> either declare itself leader or intitiate an election run
                if addr == self.leader:
                    self.elect_leader()

    def removeFailMember(self):
        # Remove the members from the failMembershipList
        with self.rlock:
            now = int(time.time())
            threshold_time = now - self.cleanup_time_threshold
            fail_members_to_remove = [member_id for member_id, fail_time in self.failMemberList.items() if fail_time < threshold_time]
            for member_id in fail_members_to_remove:
                del self.failMemberList[member_id]

    def json(self):
        # Method to generate a JSON representation of the server's membership information.
        with self.rlock:
            if self.gossipS:
            # If using GossipS protocol, include additional information like status and incarnation.
                return {
                    m['id']:{
                        'id': m['id'],
                        'addr': m['addr'],
                        'heartbeat': m['heartbeat'] ,
                        'status': m['status'],
                        'incarnation': m['incarnation']
                    }
                    for m in self.MembershipList.values()
                }
            else:
            # If not using GossipS protocol, include basic information like ID, address, and heartbeat.
                return {
                    m['id']:{
                        'id': m['id'],
                        'addr': m['addr'],
                        'heartbeat': m['heartbeat'] ,
                    }
                    for m in self.MembershipList.values()
                }

    def printMembershipList(self):
        # Method to print the membership list to the log file and return it as a string.
        with self.rlock:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            log_message = f"{timestamp} ==============================================================\n"
            log_message += f" {str(self.failMemberList)}\n"
            for member_id, member_info in self.MembershipList.items():
                log_message += f"ID: {member_info['id']}, Heartbeat: {member_info['heartbeat']}, Status: {member_info['status']}, Incarnation:{member_info['incarnation']}, Time: {member_info['time']}\n"
            with open('output.log', 'a') as log_file:
                log_file.write(log_message)
            return(log_message)


    def chooseMemberToSend(self):
        # Method to randomly choose members from the membership list to send messages to.
        with self.rlock:
            candidates = list(self.MembershipList.keys())
            random.shuffle(candidates)  # Shuffle the list in-place
            return candidates[:self.n_send]
    
    def receive(self):
        """
        A server's receiver is respnsible to receive all gossip UDP message:
        :return: None
        """
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.bind(self.addr)
            while True:
                try:
                    # UDP receiver
                    data, server = s.recvfrom(4096)
                    # * if receives data
                    if data:
                        if random.random() < self.drop_rate:
                            continue
                        else:
                            msgs = json.loads(data.decode('utf-8'))
                            # * print out the info of the received message
                            self.updateMembershipList(msgs) 
                except Exception as e:
                    traceback.print_exc()

    #MP3: send election message or elected message to all other nodes
    def election_sender(self, command, addr=None):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            if command == "elected": # process is claiming that it has been elected
                target = []
                for _, member_info in self.MembershipList.items():
                    # target.append((ip, SERVER_PORT))
                    target.append((member_info['addr'][0], LIST_PORT)) # address (ex. ('fa23-cs425-9505.cs.illinois.edu', 12360))
                for i in target:
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
                    sock.connect(i)
                    # s.sendall(("elected," + member_info['addr'][0])) # elected this
                    print(f"Sending elected message to: {member_info['addr'][0]}")
                    print("electing:", addr)
                    sock.sendall(f"elected,{addr}".encode())
                    
            elif command == "election":
                pass

    #MP3: return which IPS to send to
    def leader_get_addr(self, filename):
        return self.leader_list[filename]

    #MP3: 
        # getfile: a different node requests GET from this node -> responds with the file content
        # putfile: a differnet node requests PUT from this node -> this node will receive file content
        # deletefile: leader node requests this node to DELETE a certain file
        # ls: leader requests this node to respond "found" if it contains a certain file
    def file_server_thread(self, s, ip):        #ip denotes the source of the socket
        while True:
            hostname, _, _ = socket.gethostbyaddr(ip)
            # if not self.MembershipList[hostname]["status"] == "Alive":
            #     s.close()
            if not self.alive_in_membership_list(hostname):
                s.close()
            message = s.recv(1024).decode('utf-8')
            message = message.split(",")      # use commas to divide parts of message string

            # ex) message = getfile, local_filename
            # print("MESSAGEEEE", message)
            command = message[0]
            if command == "ls":
                if os.path.isfile('SDFS/'+message[1]) or os.path.isfile(message[1]):
                    s.sendall("found".encode('utf-8'))
                else:
                    s.sendall("not_found".encode('utf-8'))
            elif command == "multiread":
                sdfs_fname = message[1]
                local_fname = message[2]
                self.command_sender(command = "get", sdfs_fname = sdfs_fname, local_fname = os.path.join('SDFS/', local_fname))
            elif command == "deletefile":
                '''
                receive (deletefile, file_name) from the client, response success or fail
                '''
                if os.path.isfile('SDFS/'+message[1]):
                    os.remove('SDFS/'+message[1])
                    s.sendall("success".encode('utf-8'))
                else:
                    s.sendall("fail".encode('utf-8'))
            elif command == "getfile":
                '''
                receive (getfile, file_name) from the client, return the whole file
                '''
                if os.path.isfile('SDFS/'+message[1]):
                    with open("SDFS/"+message[1], "rb") as file:
                        file_size = os.path.getsize("SDFS/"+message[1])
                        buffer = str(file_size)# .encode('utf-8')
                        buffer += " " * (1024-len(buffer))
                        s.send(buffer.encode())
                        while file_size > 0:
                            s.send(file.read(1024))
                            file_size -= 1024
                    #     file_content = file.read()
                    # s.sendall(file_content.encode('utf-8'))
                else:
                    s.sendall(f"no file on {self.ip}".encode('utf-8'))
            elif command == "putfile":
                '''
                receive (putfile, file name, file content) from the client, return success or fail
                '''
                fail_flag = False
                try:
                    with open("SDFS/"+message[1], "wb") as file:
                        size = int(message[2])
                        receive = 0
                        print(f"size = {size}")
                        while receive<size:
                            buf = s.recv(1024)
                            file.write(buf)
                            receive += len(buf)
                            # print(f"received {receive} bytes")
                        print(f"received {receive} bytes in total")
                except:
                    fail_flag = True
                    s.sendall("fail".encode('utf-8'))
                if not fail_flag:
                    s.sendall("success".encode('utf-8'))
            else:   
                print(" COMMANDDD:", command, message)
                self.write_queue_lock.acquire()
                if command == "put" or command == "delete":
                    self.write_queue.put((command, message[1]))
                    print(" PUT into queueueu")
                else:
                    if len(message) > 1:
                        self.read_queue.put((command, message[1]))
                
                if self.write_queue.empty() and not self.read_queue.empty(): 
                    command, sdfs_fname = self.read_queue.get()
                elif not self.write_queue.empty() and self.read_queue.empty(): 
                    command, sdfs_fname = self.write_queue.get()
                elif self.recently_read == True:
                    command, sdfs_fname = self.write_queue.get()
                elif self.recently_read == False:
                    command, sdfs_fname = self.read_queue.get()
                if command == "put":  # LEADER RECEIVES PUT -> responds with addresses
                    sdfs_fname = message[1]
                    replicas = self.update_leader_list(sdfs_fname)
                    
                    s.send(f"success,{str(replicas)}".encode('utf-8'))

                    # MAKE SURE WE GET A RESPONSE BACK ON IF WE HAD PREVIOUSLY FINISHED WRITING 
                    while True:
                        mes = s.recv(4096).decode()
                        if mes == "finished":
                            print("DONEEE")
                            self.write_queue_lock.release()
                            break
                    self.recently_read = False
                elif command == "get": # LEADER RECEIVES GET -> responds w/ addresses containing the SDFS file OR fail
                    print("LEADER LIST:", self.leader_list)
                    sdfs_fname = message[1]
                    if sdfs_fname in self.leader_list:
                              s.send(f"success,{self.leader_list[sdfs_fname]}".encode('utf-8'))
                              self.leader_list[sdfs_fname].append(hostname)
                    else:
                              # TODO: NOT EXISTING
                              s.send(f"fail,".encode('utf-8'))
                    
                    self.recently_read = True
                    
                elif command == "delete": # LEADER RECEIVES DELETE -> removes from leader list and responds w/ addresses containing SDFS file
                    sdfs_fname = message[1]
                    if sdfs_fname in self.leader_list:
                              s.send(f"success,{self.leader_list[sdfs_fname]}".encode('utf-8'))
                              del self.leader_list[sdfs_fname]
                    else:
                              # TODO: NOT EXISTING
                              s.send(f"fail,".encode('utf-8'))
                    self.recently_read = False
                while True:
                    if s.recv(1024).decode('utf-8') == "finished":
                        break
        
        with open("leader_list.json", 'w') as f:
            json.dump(self.leader_list, f)
        s.close()
            
    def leader_list_thread(self, s, ip):
        while True:
            hostname, _, _ = socket.gethostbyaddr(ip)
            # if not self.MembershipList[hostname]["status"] == "Alive":
            #     s.close()
            try:
                if not self.alive_in_membership_list(hostname):
                    s.close()
                message = s.recv(4096).decode()
                if len(message)>0:
                    msg = message.split(',', 1)
                    if msg[0] == "list":
                        message = json.loads(msg[1])
                        
                        self.leader_list = message.copy()
                    elif msg[0] == "elected":
                        self.leader = msg[1]
                        print(self.leader, " h as been elected")
                    elif msg[0] == "election":
                        if not self.alive_in_membership_list(self.leader): # leader has been elected
                            self.elect_leader()
                # traceback.print_exc()
            except Exception as e:
                print("lost connnection with leader")
                return


    #MP3: start leader list thread
    def leader_list_server(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                print("list BIND TO:" ,(self.ip, LIST_PORT))
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((self.ip, LIST_PORT))
                s.listen(30)
                while True:
                    conn, address = s.accept()
                    print(f"new connection from {address[0]}:{ address[1]}")
                    leader_list_thread = threading.Thread(target=self.leader_list_thread, args=(conn, address[0]))
                    leader_list_thread.start()


    '''
    def leader_list_server(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                print("list BIND TO:" ,(self.ip, LIST_PORT))
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((self.ip, LIST_PORT))
                s.listen(30)
                while True:
                    conn, address = s.accept()

                    hostname, _, _ = socket.gethostbyaddr(address[0])
                    # if not self.MembershipList[hostname]["status"] == "Alive":
                    #     s.close()
                    try:
                        if not self.alive_in_membership_list(hostname):
                            s.close()
                        message = s.recv(4096).decode('utf-8')
                        if len(message)>0:
                            msg = message.split(',', 1)
                            if msg[0] == "list":
                                print("MESMGMMG", msg[1], type(msg[1]))
                                message = json.loads(msg[1])
                                print("mesggg", message)
                                self.leader_list = message
                            elif msg[0] == "elected":
                                self.leader = msg[1]
                                print(self.leader, " h as been elected")
                    except Exception as e:
                        traceback.print_exc()
                        print("lost connnection with leader")
                        return
    '''

    # MP3: start client thread
    def file_server(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                print("BIND TO:" ,(self.ip, LEADER_PORT))
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((self.ip, LEADER_PORT))
                s.listen(30)
                while True:
                    conn, address = s.accept()
                    print(f"new connection from {address[0]}:{ address[1]}")
                    client_thread = threading.Thread(target=self.file_server_thread, args=(conn, address[0]))
                    client_thread.start()

    # MP3: sends command to leader; receives which addresses to continue with; sends directly to one (ex. get) or all (ex. put) of those addresses 
    def command_sender(self, command, sdfs_fname = None, local_fname = None, vm_list = None):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            # leader = (ip, LEADER_PORT)
            leader = (self.leader, LEADER_PORT)
            if command == "ls":
                '''
                send (ls ,file_name) to each machines, receive found or not_found
                '''

                target = []
                for member_id, member_info in self.MembershipList.items():
                    if self.alive_in_membership_list(member_info['addr'][0]):
                        target.append((member_info['addr'][0], LEADER_PORT))
                result = []
                print("target", target)
                s.close()
                
                for node in target:
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.connect(node)
                    print(f"connect to server {node[0]}, operation = ls")
                    sock.sendall(("ls,"+sdfs_fname).encode('utf-8'))
                    if sock.recv(1024).decode('utf-8') == "found":
                        result.append(node[0])
                    sock.close()
                if len(result) == 0:
                    with open("log.log", "a") as file:
                        file.write("\n"+f"file {sdfs_fname} NOT FOUND ANYWHERE "+"\n")
                    print(f"file {sdfs_fname} NOT FOUND ANYWHERE")
                for r in result:
                    with open("log.log", "a") as file:
                        file.write("\n"+f"file {sdfs_fname} is found on {r}"+"\n")
                    print(f"file {sdfs_fname} is found on {r}")
                '''
                target = []
                for ip in HOST_NAME_LIST:
                    target.append((ip, SERVER_PORT))
                result = []
                for i in target:
                    s.connect(i)
                    print(f"connect to server {i[0]}, operation = ls")
                    s.sendall(("ls,"+sdfs_fname).encode('utf-8'))
                    if s.recv(1024).decode('utf-8') == "found":
                        result.append(i[0])
                for r in result:
                    with open("log.log", "a") as file:
                        file.write("\n"+f"file {sdfs_fname} is found on {r}"+"\n")
                    print(f"file {sdfs_fname} is found on {r}"+"\n")
                '''
            elif command == "delete":
                '''
                send (delete, file_name) to the leader, receive success or fail
                '''
                s.connect(leader)
                print(" DELLL")
                print(f"connect to leader {leader[0]}, operation = delete")
                s.sendall(("delete,"+sdfs_fname).encode('utf-8'))
                response = s.recv(4096).decode('utf-8').split(",", 1)

                if response[0] == "success":
                    address_list = eval(response[1])
                    
                    for addr in address_list:

                        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as d:
                            if self.alive_in_membership_list(addr):
                                # TODO: if in membership list -> send to this server
                                conn_addr = (addr, LEADER_PORT)

                                d.connect(conn_addr)
                                d.sendall(("deletefile,"+sdfs_fname).encode('utf-8'))
                                if d.recv(1024).decode('utf-8') == "success":
                                    with open("log.log", "a") as file:
                                        file.write("\n"+f"Successfully deleted file {sdfs_fname} at {addr}"+"\n")
                                    print(f"Successfully deleted file {sdfs_fname} at {addr}"+"\n")
                                else:
                                    print(f"cannot delete file {sdfs_fname} at {addr}"+"\n")
                    with open("log.log", "a") as file:
                        file.write("\n"+f"delete file {sdfs_fname}"+"\n")
                    print(f"delete file {sdfs_fname}"+"\n")
                else:
                    with open("log.log", "a") as file:
                        file.write("\n"+f"File not found: {sdfs_fname}"+"\n")
                    print(f"File not found: {sdfs_fname}")
                s.sendall(("finished").encode('utf-8'))
            elif command == "get":
                '''
                send (get, file_name) to the leader, receive (success, ip of a node that have the file)
                send (getfile, file_name) to the server, get the file
                '''
                directory = 'SDFS/'
                if os.path.exists(directory+sdfs_fname):
                    print(f"already have the file"+"\n")
                    return
                s.connect(leader)

                # send message to leader -> receives response containing list of replicas containing the ip addresses
                # will check if addresses are still in the membershiplist and if is then send one by one until we can read a file
                print(f"connect to leader {leader[0]}, operation = get")

                s.sendall(("get,"+sdfs_fname).encode('utf-8'))
                response = s.recv(4096).decode('utf-8').split(",", 1) # split only on first
                print("resp", response[1])
                if response[0] == "success":
                    # if success -> will send message to desired address
                    address_list = eval(response[1])

                    for addr in address_list:
                        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as d:
                            # TODO: ITERATE through the IPs that we received from leader and check if still in membership list 
                            if self.alive_in_membership_list(addr):
                                # TODO: if in membership list -> send to this server
                                conn_addr = (addr, LEADER_PORT)

                                d.connect(conn_addr)
                                d.sendall(("getfile,"+sdfs_fname).encode('utf-8'))
                              #   buffer = d.recv(4096).decode('utf-8')     
                                with open(local_fname, "wb") as file: # in SDFS: with open(directory+local_fname, 'w') as file:
                                        size = int(d.recv(1024).decode('utf-8'))
                                        local = 0
                                        while local<size:
                                            buf = d.recv(1024)
                                            file.write(buf)
                                            local += len(buf)
                              #       file.write(buffer)
                                # can ALSO add in SDFS file as well 
                                print(f"fetched file {sdfs_fname} from {addr}"+"\n")
                                s.sendall(("finished").encode('utf-8'))
                                break # found file
                else:
                    print("Don't have the file")
                    s.sendall(("finished").encode('utf-8'))

            elif command == "put":
                '''
                send str(put, file_name) to the leader, and receive (success, replica numbers, replica ip)
                send str(putfile, file name, file content) to the server, get(success) or (fail)
                '''
                s.connect(leader)
                print(f"connect to leader {leader[0]}, operation = put")
                # sends sdfs_fname to leader -> leader will hash this name and respond w/ 4 replicas (at most 3 failures)
                s.sendall(("put,"+sdfs_fname).encode('utf-8'))
                print("SENT: ", "put,"+sdfs_fname)
                response = s.recv(4096).decode('utf-8').split(",", 1) # split only on first

                if response[0] == 'success':
                    address_list = eval(response[1])
                    for addr in address_list:
                        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as d:
                            conn_addr = (addr, LEADER_PORT)

                            d.connect(conn_addr)
                            # with open("SDFS/"+sdfs_fname, "rb") as file:
                            with open(local_fname, "rb") as file:
                    #             file_content = file.read()
                    #         print("CONTENT ", file_content)
                            # d.sendall(("putfile,"+sdfs_fname+","+file_content).encode('utf-8'))
                              size = os.path.getsize(local_fname)
                              buffer = "putfile,"+sdfs_fname+","+str(size)+","
                              buffer += " " * (1024-len(buffer))
                              d.send(buffer.encode('utf-8'))
                              while size>0:
                                  d.send(file.read(1024))
                                  size -= 1024
                            # if was able to send file to certain node -> will receive success message
                            if not d.recv(1024).decode('utf-8') == "fail":
                                with open("log.log", "a") as file:
                                    file.write("\n"+f"put file {sdfs_fname} at {addr}"+"\n")
                                print(f"put file {sdfs_fname} at {addr}"+"\n")
                            else:
                                print(f"cannot put file {sdfs_fname} at {addr}"+"\n")
                else:
                    print("put error")
                
                s.sendall(("finished").encode('utf-8'))

            elif command == "multiread":
                for addr in vm_list:
                    conn_addr = (addr, LEADER_PORT)
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.connect(conn_addr)
                    sock.send(f"multiread,{sdfs_fname},{local_fname}".encode())
        with open("leader_list.json", 'w') as f:
            json.dump(self.leader_list, f)




    def user_input(self):
        """
        Toggle the sending process on or off.
        :param enable_sending: True to enable sending, False to disable sending.
        """
        # Files folder created here
        folder_name = "SDFS"
        if not os.path.exists(folder_name):
            os.mkdir(folder_name)
            print(f"Folder '{folder_name}' created successfully.")
        else:
            print(f"Folder '{folder_name}' already exists.")
        while True:
            user_input = input("Enter 'join' to start sending, 'restart' to rereplicate, 'leave' to leave the group, 'enable suspicion' for GOSSIP+S mode and 'disable suspicion' for GOSSIP mode, 'list_mem' to list members and 'list_self' to list self info:\n")
            # add PUT, DELETE, and GET operations here
            # 
            begin_time = time.time()
            if user_input == 'join':
                self.enable_sending = True
                print("Starting to send messages.")
                self.MembershipList = {
                f"{ip}:{port}:{self.timejoin}": {
                "id": f"{ip}:{port}:{self.timejoin}",
                "addr": (ip, port),
                "heartbeat": 0,
                "status": "Alive",
                "incarnation": 0,
                "time": time.time(),
                }
                for ip, port in [(IP, DEFAULT_PORT_NUM) for IP in [self.ip, Introducor]]
            }    
            elif user_input == "restart":
                file_list = []
                for filename in os.listdir('SDFS/'):
                    file_path = os.path.join('SDFS/', filename)
                    if os.path.isfile(file_path):
                        file_list.append(filename)               
                for filename in file_list:
                    self.command_sender(command = "get", sdfs_fname = filename, local_fname = os.path.join('SDFS/', filename))
                    print(f"restored file {filename}")

            elif user_input == 'leave':
                self.enable_sending = False
                print("Leaving the group.")
            elif user_input == 'enable suspicion':
                self.gossipS = True
                print("Starting gossip S.")
            elif user_input == 'disable suspicion':
                self.gossipS = False
                print("Stopping gossip S.")
            elif user_input == 'list_mem':
                print(self.printMembershipList())
            elif user_input == 'leader_list':
                for i in self.leader_list:
                    print(f"file {i}: node =")
                    for j in self.leader_list[i]:
                        print(f" {j}")
            elif user_input == 'list_self':
                self.printID()
            elif user_input == 'store':
                # TODO: list all files currently stored on this machine
                directory = 'SDFS/'
                
                # iterate over files in SDFS directory 
                for filename in os.listdir(directory):
                    f = os.path.join(directory, filename)
                    # checking if it is a file
                    if os.path.isfile(f):
                        print(f)
            elif len(user_input) > 2 and user_input[0:2] == "ls":           
                inp = user_input.split(" ")
                if len(inp) < 2:
                    print("ls command requires [filename].")
                    continue
                sdfs_fname = inp[1]
                self.command_sender(command = "ls", sdfs_fname = sdfs_fname)
                # FOR REST:
                '''
                - could possibly add message onto a queue wrapped w/ a mutex lock
                - SDFS server will pull message off this queue and perform the corresponding operation and then continue operating on messages
                '''
            elif len(user_input) >= 3 and user_input[0:3] == "put":

                inp = user_input.split(" ")
                if len(inp) < 3:
                    print("PUT command requires [filename].")
                    continue
                local_fname = inp[1]
                sdfs_fname = inp[2]
                directory = 'SDFS/'

                # if not os.path.exists(os.path.join(directory, local_fname)):
                if not os.path.exists(os.path.join(os.getcwd(), local_fname)):
                    print("File doesn't exist in local directory. Try again.")
                    continue

                # send message to SDFS to put file in SDFS file directory as sdfs_fname
                self.command_sender(command = "put", sdfs_fname = sdfs_fname, local_fname = local_fname)
            elif len(user_input) >= 3 and user_input[0:3] == "get":
                inp = user_input.split(" ")
                if len(inp) < 3:
                    print("GET command requires [filename].")
                    continue
                sdfs_fname = inp[1]
                local_fname = inp[2]
                # send message to SDFS server to retrieve file and copy bytes of data onto a new local file called local_fname
                self.command_sender(command = "get", sdfs_fname = sdfs_fname, local_fname = local_fname)
            elif len(user_input) >= 6 and user_input[0:6] == "delete":
                inp = user_input.split(" ")
                if len(inp) < 2:
                    print("DELETE command requires [filename].")
                    continue
                sdfs_fname = inp[1]
                self.command_sender(command = "delete", sdfs_fname = sdfs_fname)
            elif len(user_input) >= len("multiread") and user_input[0:9] == "multiread":
                inp = user_input.split(" ", 3)
                if len(inp) < 3:
                    print("MULTIREAD command requires [filename] and [VM numbers (ex. 01, 02, etc...)].")
                    continue
                sdfs_fname = inp[1]
                local_fname = inp[2]
                if len(inp[3]) < 1:
                    print("MULTIREAD command requires [filename] and [VM numbers (ex. 01, 02, etc...)].")
                    continue
                vmNums = inp[3].split(' ')
                address_list = []
                for member_id, member_info in self.MembershipList.items():
                    addr = member_info['addr'][0]
                    if self.alive_in_membership_list(addr) and addr[13:15] in vmNums:
                        address_list.append(addr)
                    
                print(f"Multiread with following vms: {vmNums}")
                self.command_sender(command = "multiread", sdfs_fname = sdfs_fname, local_fname = local_fname, vm_list = address_list)
            else:
                print("Invalid input.")
            end_time = time.time()
            print(f"total time for {user_input} is {end_time-begin_time} time units")
                
    def send(self):
        """
        A UDP sender for a node. It sends json message to random N nodes periodically
        and maintain time table for handling timeout issue.
        :return: None
        """
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            while True:
                try:
                    if self.enable_sending:  # Check if sending is enabled
                        self.update_heartbeat()
                        peers = self.chooseMemberToSend()
                        for peer in peers:
                            send_msg = self.json()
                            s.sendto(json.dumps(send_msg).encode('utf-8'), tuple(self.MembershipList[peer]['addr']))
                    time.sleep(self.protocol_period)          
                except Exception as e:
                    print(e)
                    
    def update_heartbeat(self):
        # Method to update the server's heartbeat and refresh its status in the membership list.
        with self.rlock:
            self.heartbeat += 1
            self.MembershipList[self.id]["status"] = "Alive"
            self.MembershipList[self.id]["heartbeat"] = self.heartbeat
            self.MembershipList[self.id]["time"] = time.time()
            self.MembershipList[self.id]["incarnation"] = self.incarnation
            if self.gossipS:
                self.detectSuspectAndFailMember()
            else:
                self.detectFailMember()
            self.removeFailMember()
            self.printMembershipList()

    # MP3: call this function when receiving a PUT command
    def update_leader_list(self, sdfs_fname):
        # will update leader list and store replicas on 3 other VMS
        if len(self.MembershipList) <=4:
            for member_id, member_info in self.MembershipList.items():

                id = member_info['addr'][0]
                if sdfs_fname in self.leader_list:
                    self.leader_list[sdfs_fname].append(id)
                else:
                    self.leader_list[sdfs_fname] = [id]
            for member_id, member_info in self.MembershipList.items():
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                print(" send list to:", (member_info['addr'][0],LIST_PORT))
                sock.connect((member_info['addr'][0],LIST_PORT))

                #serialize leader list dictionary
                json_str = json.dumps(self.leader_list)
                sock.sendall(f"list,{json_str}".encode('utf-8'))

            return self.leader_list[sdfs_fname]

        start_index = hash(sdfs_fname)%len(self.MembershipList)
        end_index = (hash(sdfs_fname)+4)%len(self.MembershipList)

        sorted_keys = sorted(self.MembershipList.keys())

        # Iterate through the sorted keys and access the corresponding values
        if start_index < end_index:
            for i,key in enumerate(sorted_keys):
                if start_index <= i and i < end_index:
                    member_info = self.MembershipList[key]
                    self.edit_list(member_info,sdfs_fname)
        else:
            for i,key in enumerate(sorted_keys):
                if i>=start_index or i < end_index:
                    member_info = self.MembershipList[key]
                    self.edit_list(member_info, sdfs_fname)

        print("replica vms:", sdfs_fname, (start_index, end_index), self.leader_list[sdfs_fname])
        for member_id, member_info in self.MembershipList.items():
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            sock.connect((member_info['addr'][0],LIST_PORT))

            #serialize leader list dictionary
            print(self.leader_list, type(self.leader_list))
            json_str = json.dumps(self.leader_list)
            print("sendinggg to: ",(member_info['addr'][0],LIST_PORT))
            sock.sendall("list,".encode() +json_str.encode('utf-8'))

        # ensure minimum number of replicas is 4 for all 
        for fname, vm_list in self.leader_list.items():
            if len(self.leader_list[fname]) <4:
                sock_update = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                replicas_needed = 4-len(self.leader_list[fname])
                for member_id, member_info in self.MembershipList.items():
                    if member_info['addr'][0] not in self.leader_list[fname] and self.alive_in_membership_list(member_info['addr'][0]):
                        # send to this addr
                        conn_addr = (member_info['addr'][0],LEADER_PORT)
                        sock_update.connect(conn_addr)
                        if not os.path.exists("local_storage.bin"):
                            self.command_sender("get", sdfs_fname=fname, local_fname="local_storage.bin")
                        
                        with open("local_storage", "rb") as file:
                #             file_content = file.read()
                #         print("CONTENT ", file_content)
                        # d.sendall(("putfile,"+sdfs_fname+","+file_content).encode('utf-8'))
                            size = os.path.getsize(local_fname)
                            buffer = "putfile,"+sdfs_fname+","+str(size)+","
                            buffer += " " * (1024-len(buffer))
                            sock_update.send(buffer.encode('utf-8'))
                            while size>0:
                                sock_update.send(file.read(1024))
                                size -= 1024
                        replicas_needed-=1
                        self.leader_list[fname].append(member_info['addr'][0])

                    if replicas_needed == 0:
                        break


        return self.leader_list[sdfs_fname]
            
        # for i in range(len(self.MembershipList.keys())):
        #     if start_index <= i and i < end_index:
        #         # update leader list here as well
        #         key = self.membershipList.keys()[i]
        #         member_info = self.membershipList[key]
        #         if sdfs_fname in self.leader:
        #             self.leader_list[sdfs_fname].append(member_info['id'])
        #         else:
        #             self.leader_list[sdfs_fname] = [member_info['id']]
                
                # TODO (not): send files to following replicas 
        '''
        if sdfs_fname in self.leader:
            self.leader_list[sdfs_fname].append(id)
        else:
            self.leader_list[sdfs_fname] = [id] # store ID and local filename (on vms)
        self.store_replicas(sdfs_fname)
        '''
    def edit_list(self, member_info, sdfs_fname):
        id = member_info['addr'][0]
        if sdfs_fname in self.leader_list:
            self.leader_list[sdfs_fname].append(id)
        else:
            self.leader_list[sdfs_fname] = [id]

    # MP3: MIGHT NOT NEED (only need elect_leader)
    def call_election(self):
        # send election message to all members in group -> need new leader
        pass
    
    # MP3: will either declare itself leader or send election message out 
    def elect_leader(self):
        lowest = True
        print("LEADER DEAD! ELECTING LEADER!", len(self.MembershipList))
        # iterate through all members in group
        # for _, member_info in self.MembershipList.items():
        #     print("emmb info", member_info)
        for _, member_info in self.MembershipList.items(): 
            ip = member_info['addr'][0]
            if int(ip[13:15]) < self.id_num:
                # CANT be leader
                # send election mesage to all nodes in membership list
                self.election_sender("election")
                lowest = False
        
        if lowest:
            print("MYSELFFF", socket.gethostname())
            
            self.election_sender("elected", socket.gethostname())
            # send leader message to all members in group and declare itself

    # MP3: will update the leader list in the following replicas
    def store_replicas(self, sdfs_fname, local_fname):
        print("GETTING RPLLICAS")
        start_index = hash(sdfs_fname)%len(self.membershipList)
        end_index = (hash(sdfs_fname)+3)%len(self.membershipList)

        if start_index > end_index:
            # flip indicies
            temp = end_index
            end_index = start_index
            start_index = temp
            
        for i in range(len(self.membershipList.keys())):
            if start_index <= i and i < end_index:
                # update leader list here as well
                key = self.membershipList.keys()[i]
                member_info = self.membershipList[key]
                if sdfs_fname in self.leader:
                    self.leader_list[sdfs_fname].append((member_info['id'], local_fname))
                else:
                    self.leader_list[sdfs_fname] = [(member_info['id'], local_fname)]
                
                # TODO (not): send files to following replicas (probably not this)
    
    # MP3: new leader should be updated
    def received_leader_message(self, new_leader):
        self.leader = new_leader

    # MP3: checks if an address should be in the membership list
    def alive_in_membership_list(self, address):
        for member_id, member_info in self.MembershipList.items():
            if member_id.find(address) != -1:
                if self.MembershipList[member_id]['status'] == "Alive":
                    return True
        return False

    def run(self):
        """
        Run a server as a node in group.
        There are totally two parallel processes for a single node:
        - receiver: receive all UDP message
        - sender: send gossip message periodically

        :return: None
        """
        
        receiver_thread = threading.Thread(target=self.receive)
        receiver_thread.daemon = True
        receiver_thread.start()

        # Start a sender thread
        sender_thread = threading.Thread(target=self.send)
        sender_thread.daemon = True
        sender_thread.start()

        # Start a to update enable sending
        user_thread = threading.Thread(target=self.user_input)
        user_thread.daemon = True
        user_thread.start()

        # self.file_server()
        # self.leader_list_server()

        file_thread = threading.Thread(target=self.file_server)
        file_thread.daemon = True
        file_thread.start()

        leader_thread_list = threading.Thread(target=self.leader_list_server)
        leader_thread_list.daemon = True
        leader_thread_list.start()

        receiver_thread.join()
        sender_thread.join()
        user_thread.join()
        leader_thread_list.join()
        file_thread.join()
        


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--protocol-period', type=float, help='Protocol period T in seconds', default=0.25)
    parser.add_argument('-d', '--drop-rate', type=float,
                        help='The message drop rate',
                        default=0)
    args = parser.parse_args()
    
    server = Server(args)
    server.run()
