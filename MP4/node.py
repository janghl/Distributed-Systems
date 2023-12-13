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
import subprocess
import pandas as pd
# Define a list of host names that represent nodes in the distributed system.
# These host names are associated with specific machines in the network.
# The 'Introducer' variable points to a specific host in the system that may serve as an introducer node.

# HOST_NAME_LIST = [
#     'fa23-cs425-6901.cs.illinois.edu',
#     'fa23-cs425-6902.cs.illinois.edu',
#     'fa23-cs425-6903.cs.illinois.edu',
#     'fa23-cs425-6904.cs.illinois.edu',
#     'fa23-cs425-6905.cs.illinois.edu',
#     'fa23-cs425-6906.cs.illinois.edu',
#     'fa23-cs425-6907.cs.illinois.edu',
#     'fa23-cs425-6908.cs.illinois.edu',
#     'fa23-cs425-6909.cs.illinois.edu',
#     'fa23-cs425-6910.cs.illinois.edu',
# ]
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
MAPLE_PORT = 8040
JUICE_PORT = 8060
FILE_PORT = 8000
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
        self.leader = 'fa23-cs425-9502.cs.illinois.edu' # init leader to VM 2
        self.replica_leaders = ['fa23-cs425-6901.cs.illnois.edu', 'fa23-cs425-6903.cs.illnois.edu', 'fa23-cs425-6904.cs.illnois.edu']
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
                    target.append((member_info['addr'][0], LIST_PORT)) # address (ex. ('fa23-cs425-6905.cs.illinois.edu', 12360))
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

    def file_port(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                print("BIND TO:" ,(self.ip, FILE_PORT))
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((self.ip, FILE_PORT))
                s.listen(30)
                while True:
                    conn, address = s.accept()
                    print(f"new file port connection from {address[0]}:{ address[1]}")
                    client_thread = threading.Thread(target=self.file_port_thread, args=(conn, address[0]))
                    client_thread.start()

    def file_port_thread(self, s, ip):        #ip denotes the source of the socket
        while True:
            hostname, _, _ = socket.gethostbyaddr(ip)
            # if not self.MembershipList[hostname]["status"] == "Alive":
            #     s.close()

            if not self.alive_in_membership_list(hostname):
                s.close()
            try:
                msg = s.recv(8000).decode('utf-8') # padding for when metadata is longer (ex. append)
            except Exception as e:
                print("ERROR", e)
            if len(msg) == 0:
                continue
            # socket_status = s.getsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE)

            # # Print the socket status
            # print(f"Socket Status: {socket_status}")
            message = msg.split(",") 
            command = message[0]  
            if command == "putfile":
                '''
                receive (putfile, file name, file content) from the client, return success or fail
                '''
                fail_flag = False
                message = msg.split(',',3)
                print("putting file now....")
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
            elif command == "getfile":
                '''
                receive (getfile, file_name) from the client, return the whole file
                '''
                
                if os.path.isfile('SDFS/'+message[1]):
                    print("GETTING FILE", message[1])
                    with open("SDFS/"+message[1], "rb") as file:
                        file_size = os.path.getsize("SDFS/"+message[1])
                        buffer = str(file_size)# .encode('utf-8')
                        buffer += " " * (8000-len(buffer))
                        s.send(buffer.encode())
                        while file_size > 0:
                            s.send(file.read(1024))
                            file_size -= 1024
                    #     file_content = file.read()
                    # s.sendall(file_content.encode('utf-8'))
                else:
                    s.sendall(f"no file on {self.ip}".encode('utf-8'))


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
            try:
                msg = s.recv(8000).decode('utf-8') # padding for when metadata is longer (ex. append)
            except Exception as e:
                print("ERROR", e)
            if len(msg) == 0:
                continue
            
            message = msg.split(",")      # use commas to divide parts of message string
            if (len(message) == 1 and message[0] == "finished"):
                continue
            print("MESSAGE RECEIVED:::", message)
            if "sdfs_dest_output" in message[1]:
                print("APPENDING OUTPUTTTTTTT\n\n\n\n") 
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

            elif command == "maple":
                
                # num_maples = number of maple tasks being run 
                    # ex) if num_maples = 3 -> split among 3 vms
                    # if num_maples > # of VMS -> split among num maples and once vms finish they will begin another maple task
                    # split files into num_maples different maple tasks -> 
                # intermediate files: sdfs_intermediate_filename_prefix_KEY.txt -> the intermediate files will contain rows of the key name and "1" (mapping)
                    # rows in file have (key, value) format 
                #vm = threading.Thread(target=self.getOpenVM_maple, args=(vms_running_maple, fname, maple_exe, sdfs_intermediate_filename_prefix))
                

                if len(message) <= 5:
                    # print("sdf:::", msg.split(",",2)[1])
                    if msg.split(",",2)[1] == "filter_maple.py":
                        message = msg.split(",",2)
                        # _,maple_exe, num_maples, sdfs_intermediate_filename_prefix, dataset, regex_condition = message
                        _, maple_exe, query = message
                        thread_list = []
                        alive_machine_number = 0
                        vms_running_maple = []
                        vms_use = 0
                        for member_id, member_info in self.MembershipList.items():
                            if self.alive_in_membership_list(member_info['addr'][0]):
                                alive_machine_number += 1
                                vms_running_maple.append(member_info['addr'][0])
                                vms_use +=1
                        for i in range(len(self.MembershipList)):
                            vm = threading.Thread(target=self.getOpenVM_maple, args=(vms_running_maple, 1, None, None, None, query))
                            thread_list.append(vm)
                            vm.start()
                        # wait for all maple tasks to finish
                        for tl in thread_list:
                            tl.join()
                        s.send("finished".encode())
                        return
                    _, maple_exe, num_maples, sdfs_intermediate_filename_prefix, sdfs_src_directory = message
                    field = None
                else: # SQL QUERY
                    _, maple_exe, num_maples, sdfs_intermediate_filename_prefix, sdfs_src_directory, field = message
                    '''
                    _, maple_exe, num_maples, sdfs_intermediate_filename_prefix, sdfs_src_directory, dataset, regex_condition = message
                    field,string = regex_condition.split("=")
                    # split up/ shard data for mulitple VMs to work on mapping (maple) this dataset
                    total_num_rows = len(pd.read_csv(dataset))
                    
                    start_line = 0
                    end_line = 0
                    thread_list = []
                    # keep list of VMs running and the state if currently runing a maple/juiec job or not
                    for i in range(self.MembershipList):
                        vm = threading.Thread(target=self.getOpenVM_maple, args=(vms_running_maple, dataset, maple_exe, sdfs_intermediate_filename_prefix, start_line, end_line))
                        thread_list.append(vm)
                        vm.start()
                    # wait for all maple tasks to finish
                    for tl in thread_list:
                        tl.join()
                    return
                    '''
                # leader receives this

                #TODO:
                # get a list of all filenames with sdfs_src_directory prefix
                num_maples = int(num_maples)
                filenames = []
                print("NUM maples:", num_maples)
                for member_id, member_info in self.MembershipList.items():
                    if self.alive_in_membership_list(member_info['addr'][0]):
                        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        sock.connect((member_info['addr'][0], MAPLE_PORT))
                        sock.sendall(("getdirectoryfiles,"+sdfs_src_directory).encode('utf-8'))
                        response = sock.recv(4096).decode('utf-8')
                        response = response.split(',')
                        if response[0] == "success":
                            filenames += eval(response[1])
                        else:
                            # fail
                            pass
                        sock.close()

                directory = 'SDFS/'
                
                # iterate over files in SDFS directory 
                for filename in os.listdir(directory):
                    f = os.path.join(directory, filename)
                    # checking if it is a file
                    if os.path.isfile(f) and filename.startswith(sdfs_src_directory):
                        filenames.append(filename)

                # list of unique filenames w/ directory prefix 
                filenames = set(filenames)
                
                # use set()

                # split up evenly among vms -> use all vms if num_maples > # alive vms (possibly cap it at number of alive vms)
                # send message consisting of files and line numbers to run maple tasks on (./maple.exe)

                # number of VMs that will be used in entire maple task
                vms_use = num_maples
                alive_machine_number = 0
                vms_running_maple = [] # Hi, Joey! I made this change, vms_running_maple contains all ips of alive nodes.
                for member_id, member_info in self.MembershipList.items():
                    if self.alive_in_membership_list(member_info['addr'][0]):
                        alive_machine_number += 1
                        vms_running_maple.append(member_info['addr'][0])
                if num_maples > alive_machine_number: # possibly check if alive as well
                    vms_use = alive_machine_number
                
                '''
                start_index = hash(sdfs_fname)%len(self.MembershipList)
                end_index = (hash(sdfs_fname)+vms_use)%len(self.MembershipList)
                sorted_keys = sorted(self.MembershipList.keys())
                vms_running_maple = [] # list of VMs running the maple task
                if start_index < end_index:
                    for i,key in enumerate(sorted_keys):
                        if start_index <= i and i < end_index:
                            vms_running_maple.append(key)
                else:
                    for i,key in enumerate(sorted_keys):
                        if i>=start_index or i < end_index:
                            vms_running_maple.append(key)
                '''
                
                # for vm in vms_running_maple:
                print(filenames)
                print("vms running", vms_running_maple)
                for fname in filenames:
                    # get open VM
                    # start thread for each VM here for each filename
                    #Hi, Joey! getOpenVM is a background function where each task is assigned a thread!
                    # could possibly have each that we want running maple tasks run this function in the background 
                    if field:
                        vm = threading.Thread(target=self.getOpenVM_maple, args=(vms_running_maple, fname, maple_exe, sdfs_intermediate_filename_prefix, field))
                    else:
                        vm = threading.Thread(target=self.getOpenVM_maple, args=(vms_running_maple, fname, maple_exe, sdfs_intermediate_filename_prefix))
                    vm.start()
            elif command == "juice":
                # get all filenames with sdfs_intermediate_filename_prefix
                # assign each VM to filename and continue after finished
                
                # Input: sdfs intermediate files -> each row is (key, value) format
                # Output: sdfs_dest_filename (one file)

                _, juice_exe, num_juices, sdfs_intermediate_filename_prefix, sdfs_dest_filename, delete_input, partition_method = message
                num_juices = int(num_juices)
                vms_use = num_juices
                if num_juices > len(self.MembershipList): # possibly check if alive as well
                    vms_use = len(self.MembershipList)

                sdfs_intermediate_filename_list = []

                for member_id, member_info in self.MembershipList.items():
                    if self.alive_in_membership_list(member_info['addr'][0]):
                        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        sock.connect((member_info['addr'][0], MAPLE_PORT))
                        sock.sendall(("getdirectoryfiles,"+sdfs_intermediate_filename_prefix).encode('utf-8'))
                        response = sock.recv(10000).decode('utf-8')
                        response = response.split(',',1)
                        # print('respspp', response)
                        if response[0] == "success":
                            sdfs_intermediate_filename_list += eval(response[1])
                        else:
                            # fail
                            pass
                        

                        sock.close()
                # list of unique filenames w/ directory prefix 
                sdfs_intermediate_filename_list = set(sdfs_intermediate_filename_list)
                '''
                start_index = hash(sdfs_fname)%len(self.MembershipList)
                end_index = (hash(sdfs_fname)+vms_use)%len(self.MembershipList)
                sorted_keys = sorted(self.MembershipList.keys())
                vms_running_juice = [] # list of VMs running the juice task
                if start_index < end_index:
                    for i,key in enumerate(sorted_keys):
                        if start_index <= i and i < end_index:
                            vms_running_juice.append(key)
                else:
                    for i,key in enumerate(sorted_keys):
                        if i>=start_index or i < end_index:
                            vms_running_juice.append(key)

                '''
                alive_machine_number = 0
                vms_running_juice = [] 
                for member_id, member_info in self.MembershipList.items():
                    if self.alive_in_membership_list(member_info['addr'][0]):
                        alive_machine_number += 1
                        vms_running_juice.append(member_info['addr'][0])
                if num_juices > alive_machine_number: # possibly check if alive as well
                    vms_use = alive_machine_number

                # get all the filenames w/ the SDFS Intermediate filename prefix
                # range-based partiionining on the keys -> assign portion of keys to each VM (worker node) - each sdfs intermidate file is a key

                partitions = []
                sdfs_intermediate_filename_list = list(sdfs_intermediate_filename_list)
                
                if partition_method == "range": # range based partioning
                    keys_per_partition = len(sdfs_intermediate_filename_list) // len(vms_running_juice)
                    # Calculate the number of remaining keys after creating equal-sized partitions
                    remaining_keys = len(sdfs_intermediate_filename_list) % len(vms_running_juice)
                    
                    for i,vm in enumerate(vms_running_juice):
                        # Calculate the starting index for each partition
                        start_index = i * keys_per_partition
                        # Calculate the ending index for each partition
                        end_index = start_index + keys_per_partition + (1 if i < remaining_keys else 0)
                        # Append the partition to the list of partitions
                        partitions.append(sdfs_intermediate_filename_list[start_index:end_index])
                else: # hash based partioning
                    for i in range(vms_running_juice):
                        partitions.append([])
                    for int_file in sdfs_intermediate_filename_list:
                        index = hash(int_file)%vms_running_juice # hash the filename and put filename in corresponding index based off of hashed value
                        partitions[index].append(int_file)

                # filename_keys = list of keys that a juice task will be assigned

                for i,filename_keys in enumerate(partitions):
                    # get open VM
                    # start thread for each VM here for each filename
                    #Hi, Joey! getOpenVM is a background function where each task is assigned a thread!
                    # could possibly have each that we want running maple tasks run this function in the background 
                    vm = threading.Thread(target=self.getOpenVM_juice, args=(vms_running_juice, juice_exe, filename_keys, sdfs_dest_filename, delete_input, sdfs_intermediate_filename_prefix, i))
                    vm.start()
                return


            else:
                # print(" COMMANDDD:", command, message)
                
                
                
                if command == "put" or command == "delete" or command == "append":
                    print("attempting to aqcuire lock")
                    self.write_queue_lock.acquire()
                    print("sucess acquire lock")
                    self.write_queue.put((command, message[1]))
                    print(" PUT into queueueu")
                else:
                    if len(message) > 1:
                        self.read_queue.put((command, message[1]))
                if self.write_queue.empty() and self.read_queue.empty():
                    self.write_queue_lock.release()
                    continue
                if self.write_queue.empty() and not self.read_queue.empty(): 
                    command, sdfs_fname = self.read_queue.get()
                elif not self.write_queue.empty() and self.read_queue.empty(): 
                    command, sdfs_fname = self.write_queue.get()
                elif self.recently_read == True:
                    command, sdfs_fname = self.write_queue.get()
                elif self.recently_read == False:
                    command, sdfs_fname = self.read_queue.get()
                if command == "put":  # LEADER RECEIVES PUT -> responds with addresses
                    print("ARRIVED HERE")
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
                    # print("LEADER LIST:", self.leader_list)
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
                elif command == "append":
                    message = msg.split(",", 2)
                    sdfs_fname = message[1]
                    additions = eval(message[2]) # list of tuple pairs

                    # send replicas -> worker node will get a replica and put file (overwrite)
                    resp = self.command_sender(command = "ls", sdfs_fname = sdfs_fname)

                    if len(resp) == 0: # not a file
                        print("NONE UET")
                        # create a file and send it out 
                        with open("temp_destination.txt", 'w') as f:
                            for pair in additions:
                                f.write(str(pair) +'\n')
                        self.write_queue_lock.release()
                        self.command_sender(command = "put", sdfs_fname = sdfs_fname, local_fname = "temp_destination.txt")
                        # try:
                            # os.remove("temp_destination.txt")
                        # except Exception as e:
                        #     print("ERROR HERE: \n\n\n", e)
                        # self.write_queue_lock.acquire()
                    else:
                        # self.command_sender(command = "get", sdfs_fname = sdfs_fname, local_fname = "temp_destination.txt")
                        # print("fateched temp destination locally")
                        contents = ""
                        with open("temp_destination.txt", 'r') as f:
                            contents = f.read()
                        # with open("temp_destination.txt", 'a') as f:
                        #     for pair in additions:
                        #         f.write(str(pair) +'\n')
                        for pair in additions:
                            contents += (str(pair) +'\n')
                        # contents = ""
                        # with open("temp_destination.txt", 'r') as f:
                        #     contents = f.read()
                        print("updated contents:", contents)
                        local_fname = "final_temp_destination.txt"
                        with open(local_fname, 'w') as f:
                            f.write(contents)
                        for r in resp:
                            with open(local_fname, "rb") as file:
                                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                                sock.connect((r, FILE_PORT)) 
                                size = os.path.getsize(local_fname)
                                buffer = "putfile,"+sdfs_fname+","+str(size)+","
                                buffer += " " * (8000-len(buffer))
                                sock.send(buffer.encode('utf-8'))
                                while size>0:
                                    sock.send(file.read(1024))
                                    size -= 1024
                        self.write_queue_lock.release()
                                # sock.send(msg_str.encode())
                        # os.remove("temp_destination.txt")
                    # self.write_queue_lock.release()
                    
                            
                # while True:
                #     if s.recv(1024).decode('utf-8') == "finished":
                #         break
        
        with open("leader_list.json", 'w') as f:
            json.dump(self.leader_list, f)
        s.close()

    # def getOpenVM(self, vm_list):
    #     for vm in vm_list:
    #         # check if VM is in uqe -> if not return that VM number
    #         pass
    # TODO: check when num_maples < total alive VMs
    def getOpenVM_maple(self, vms_running_maple=None, fname=None, maple_exe=None, sdfs_intermediate_filename_prefix=None, field=None, query=None):
        try:
            # make one do all

            if query:
                fname = query.split(" ")[3]
                target_machine_number = hash(fname)%(len(self.MembershipList))
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.connect((vms_running_maple[target_machine_number], MAPLE_PORT)) # possibly send on new port (MAPLE_PORT) -> where maple and juice tasks will be run
                msg_str = f"mapleexe,query,{query}"
                print("sendinggg:", msg_str)
                sock.send(msg_str.encode())
                resp = sock.recv(4096).decode()
                if resp == "finished":
                    return
            else:
                target_machine_number = hash(fname)%len(vms_running_maple)
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.connect((vms_running_maple[target_machine_number], MAPLE_PORT)) # possibly send on new port (MAPLE_PORT) -> where maple and juice tasks will be run
                msg_str = f"mapleexe,{maple_exe},{fname},{sdfs_intermediate_filename_prefix}"
                if field: 
                    msg_str = f"mapleexe,{maple_exe},{fname},{sdfs_intermediate_filename_prefix},{field}"
                print("sendinggg:", msg_str)
                sock.send(msg_str.encode())
                resp = sock.recv(4096).decode()
                if resp == "finished":
                    print(f"the {target_machine_number}th vm {vms_running_maple[target_machine_number]} has generated {sdfs_intermediate_filename_prefix}_{fname}")
        except Exception as e:
            print(f"failure! the {target_machine_number}th vm {vms_running_maple[target_machine_number]} CANNOT generated {sdfs_intermediate_filename_prefix}_{fname}")
    
    def getOpenVM_juice(self, vms_running_juice, juice_exe, filename_keys, sdfs_dest_filename, delete_input, sdfs_intermediate_filename_prefix, i):
        try:
            # additions = []
            # for name in filename_keys:
            # print("HEREEE")
            # target_machine_number = hash(filename_keys[0])%len(vms_running_juice)
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((vms_running_juice[i], JUICE_PORT)) # possibly send on new port (JUICE_PORT) -> where maple and juice tasks will be run 
            msg_str = f"juiceexe,{juice_exe},{sdfs_dest_filename},{delete_input},{sdfs_intermediate_filename_prefix},{filename_keys}"
            sock.send(msg_str.encode())
            resp = sock.recv(4096).decode()
            resp = resp.split(',')
            if resp[0] == "finished":
                pass
            # print(f"the {target_machine_number}th vm {vms_running_maple[target_machine_number]} has generated {sdfs_intermediate_filename_prefix}_{fname}")
        except Exception as e:
            print(e)
            # print(f"failure! the {i}th vm {vms_running_juice[i]} CANNOT generated {sdfs_intermediate_filename_prefix}_{name}")
    
            
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

    def maple_func(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                print("BIND TO:" ,(self.ip, MAPLE_PORT))
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((self.ip, MAPLE_PORT))
                s.listen(1) # only one at a time
                while True:
                    conn, address = s.accept()
                    print(f"new connection from {address[0]}:{ address[1]}")
                    # OR only allow one connection at a time -> add other maple requests onto a queue and wait
                    client_thread = threading.Thread(target=self.maple_func_thread, args=(conn, address[0]))
                    client_thread.start()

    # upon a worker node receiving a maple command from leader
    def maple_func_thread(self, s, ip):
        msg = s.recv(1024).decode('utf-8')
        message = msg.split(",")      
        
        command = message[0]

        if command == "mapleexe":
            if message[1] == "query":
                query = msg.split(",",2)[2]
                sub_command = []
                if ',' not in query.split(' ')[3]: #filter
                    query = query.replace(" ", "^")
                    print("query:",query)
                    
                    sub_command = ["python3", "filter_maple.py", "sdfs_dest_output", query]
                else: # join
                    pass
                
                # generate a bunch of files per key
                # returns a list of generated file names

                # result = subprocess.run(sub_command, stdout=subprocess.PIPE)
                # new_files = result.stdout.splitlines()
                # print(new_files)
            else:
                # run mapleexe file on corresponding file and filenums
                executable = message[1]
                fname = message[2]
                sdfs_intermediate_filename_prefix = message[3]
                field= None
                if len(message) >4:
                    field = message[4]
                #Hi, Joey, here we first fetch the file, exec it, and upload to sdfs
                # we write to the SDFS intermediate prefix directory
                self.command_sender(command = "get", sdfs_fname = fname, local_fname = fname)

                sub_command = ["python3", executable, fname, sdfs_intermediate_filename_prefix]
                if field:
                    sub_command.append(field)
                # generate a bunch of files per key
                # returns a list of generated file names
            result = subprocess.run(sub_command, stdout=subprocess.PIPE)
            new_files = result.stdout.splitlines()
            #try to get the file from sdfs, and update that
            # TODO: store locally through maple_exe file and then push to SDFS file at end -> leader will store in queue and worker nodes 
                # will be allowed access to write to SDFS files 

            # each "file" will be the sdfs_intermediate file

            # for file in new_files:
            #     self.command_sender(command = "get", sdfs_fname = file, local_fname = f"temp_{file}")
            #     if os.path.isfile(f"SDFS/temp_{file}"):
            #         with open(f"SDFS/temp_{file}", 'a') as f:
            #             with open(file, 'r') as f1:
            #                 for line in f1:
            #                     f.append(line)
            #         print(f"append to {file} on SDFS and put back")
            #         self.command_sender(command = "put", sdfs_fname = file, local_fname = f"temp_{file}")
            #         os.remove(f"SDFS/temp_{file}")
            #     else:
            #         self.command_sender(command = "put", sdfs_fname = file, local_fname = file)
            print("NEW FILES:", new_files)
            for file in new_files:
                file = file.decode()
                print(file, type(file), file, f"temp_{file}")
                self.command_sender(command = "put", sdfs_fname = file, local_fname = f"temp_{file}")
                os.remove(f"temp_{file}")
                    
            s.send("finished".encode())
            
        elif command == "getdirectoryfiles":
            print("RECEIVED getdirectoryfiles command!!")
            directory = 'SDFS/'
            directory_prefix = message[1]

            # get all filenames with the desired sdfs directory prefix
            directory_filename_list = []
            try:
                for filename in os.listdir(directory):
                    if filename.startswith(directory_prefix):
                        f = os.path.join(directory, filename)
                        # checking if it is a file
                        if os.path.isfile(f):
                            directory_filename_list.append(filename)
                directory_str = str(directory_filename_list)

                s.send(f"success,{directory_str}".encode())
            except:
                s.send(f"fail".encode())


    def juice_func(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                print("BIND TO:" ,(self.ip, JUICE_PORT))
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((self.ip, JUICE_PORT))
                s.listen(30)
                while True:
                    conn, address = s.accept()
                    print(f"new connection from {address[0]}:{ address[1]}")
                    # OR only allow one connection at a time -> add other maple requests onto a queue and wait
                    client_thread = threading.Thread(target=self.juice_func_thread, args=(conn, address[0]))
                    client_thread.start()

    # upon a worker node receiving a maple command from leader
    def juice_func_thread(self, s, ip):
        message = s.recv(10000).decode('utf-8')
        message = message.split(",",5)      
        
        command = message[0]

        # here we will also push the local file that was written to by juice_exe and push it to the SDFS that will be in the leader queue (written to replicas here)
        if command == "juiceexe":
            # run juiceexe file on corresponding file and filenums
            print("executing juicexe...")
            executable = message[1]
            sdfs_dest_filename = message[2]
            delete_input = int(message[3])
            sdfs_intermediate_filename_prefix = message[4]
            filename_keys = eval(message[5])
            additions = []

            for intermediate_file in filename_keys:
                print("sending get...", intermediate_file)
                self.command_sender(command = "get", sdfs_fname = intermediate_file, local_fname = intermediate_file, sdfs_get_flag=True)
                while not os.path.exists("SDFS/" + intermediate_file): # wait until received file
                    continue
                key = intermediate_file[len(sdfs_intermediate_filename_prefix)+1:]
                print("INT:", intermediate_file)
                sub_command = ["python3", executable, intermediate_file, key]
                result = subprocess.run(sub_command, stdout=subprocess.PIPE)
                key_val_pair = result.stdout
                key_val_pair = key_val_pair.decode()[:-1]
                print("PAIR:", key_val_pair)
                additions.append(key_val_pair)

            # additions = list of tuple pairs to add to destination file 
            # TODO: APPENDINGTO DESTINATION FILE
            if len(additions) > 0:
                self.command_sender(command = "append", sdfs_fname = sdfs_dest_filename, additions=str(additions))
            
            if delete_input == 1:
                for intermediate_file in filename_keys:
                    self.command_sender(command = "delete", sdfs_fname=intermediate_file)

            # self.command_sender(command = "put", sdfs_fname = intermediate_fname, local_fname = intermediate_fname)
            
            s.send("finished".encode())
            
        elif command == "getdirectoryfiles":
            directory = 'SDFS/'
            directory_prefix = command[1]
            # get all filenames with the desired sdfs directory prefix
            directory_filename_list = []
            try:
                for filename in os.listdir(directory):
                    if filename.startswith(directory_prefix):
                        f = os.path.join(directory, filename)
                        # checking if it is a file
                        if os.path.isfile(f):
                            directory_filename_list.append(filename)
                
                s.send(f"success,{str(directory_filename_list)}".encode())
            except:
                s.send(f"fail".encode())



    # MP3: sends command to leader; receives which addresses to continue with; sends directly to one (ex. get) or all (ex. put) of those addresses 
    def command_sender(self, command, sdfs_fname = None, local_fname = None, vm_list = None, maple_exe=None, num_maples=None, juice_exe=None, num_juices=None, sdfs_intermediate_filename_prefix=None, sdfs_src_directory=None, sdfs_dest_filename=None, delete_input=None, partition_method=None, additions=None, sdfs_get_flag=False, field=None, query=None):
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
                return result
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
                if sdfs_get_flag and os.path.exists(directory+sdfs_fname):
                    print(f"already have the file"+"\n")
                    return

                if os.path.exists(local_fname):
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
                            d = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                            # TODO: ITERATE through the IPs that we received from leader and check if still in membership list 
                            if self.alive_in_membership_list(addr):
                                # TODO: if in membership list -> send to this server
                                conn_addr = (addr, FILE_PORT) # change to file port

                                d.connect(conn_addr)
                                d.send(("getfile,"+sdfs_fname).encode('utf-8'))
                              #   buffer = d.recv(4096).decode('utf-8')
                                file_path = ""
                                if sdfs_get_flag:
                                    file_path += "SDFS/"  
                                with open(file_path+local_fname, "wb") as file: # in SDFS: with open(directory+local_fname, 'w') as file:
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
                    print("addreess list respondse", address_list)
                    for addr in address_list:
                        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as d:
                            conn_addr = (addr, FILE_PORT)

                            d.connect(conn_addr)
                            # with open("SDFS/"+sdfs_fname, "rb") as file:
                            print("CONNECTED TO FILE PORT")
                            with open(local_fname, "rb") as file:
                    #             file_content = file.read()
                    #         print("CONTENT ", file_content)
                            # d.sendall(("putfile,"+sdfs_fname+","+file_content).encode('utf-8'))
                              size = os.path.getsize(local_fname)
                              buffer = "putfile,"+sdfs_fname+","+str(size)+","
                              buffer += " " * (8000-len(buffer))
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
            elif command == "maple":
                '''
                maple_exe = maple executable -> takes in one file as input
                num_maples = number of maple tasks to be run
                    - each tasks receives ame amount of input
                sdfs_src_directory = location of input files
                sdfs_intermediate_filename_prefix_K = location of output of maple tasks (for specific )
                    - for key K -> all (K, any_value) pairs output by a Maple task written to this file
                
                '''
                
                s.connect(leader)
                print(f"connect to leader {leader[0]}, operation = maple")
                # send to leader
                if field:
                    s.sendall((f"maple,{maple_exe},{num_maples},{sdfs_intermediate_filename_prefix},{sdfs_src_directory},{field}").encode('utf-8'))
                else:
                    s.sendall((f"maple,{maple_exe},{num_maples},{sdfs_intermediate_filename_prefix},{sdfs_src_directory}").encode('utf-8'))
            elif command == "juice":
                '''
                juice_exe = juice executable -> takes input multiple (key,val) input lines
                    - process groups of (key, V) input lines together 
                    - all lines with same key processed together
                num_juices = number of juice tasks to be run
                    - each node assigned a key -> once finished wiht that key -> ready for next key
                sdfs_intermediate_filename_prefix_K = juice task processes SDFS files with associated key K
                sdfs_dest_filename = output appended to this file
                delete_input = when set to 1 -> MapleJuice deletes inputfiles automatically after Juice phase
                partition_method = range or hash
                '''
                s.connect(leader)
                print(f"connect to leader {leader[0]}, operation = juice")
                # send to leader
                s.sendall((f"juice,{juice_exe},{num_juices},{sdfs_intermediate_filename_prefix},{sdfs_dest_filename},{delete_input},{partition_method}").encode('utf-8'))

                # assign 
                # findOpenNode()
                # worker node appends output to destination file
                
            elif command.lower() == "select":
                # select_query = query.lower()
                # dataset = ""
                # regex_condition = ""
                # num_vms = len(self.MembershipList)
                s.connect(leader)

                # # if ',' in select_query: # join
                # #     pass
                # # else:
                # _,_,_,dataset,_,regex_condition = select_query.split(" ",5)
                # if len(dataset.split(','))> 1:
                #     pass # join

                # s.sendall((f"maple,filter_maple.py,{num_vms},SQL_intermediate_query_prefix,{dataset},{regex_condition}").encode('utf-8'))
                test_for_two_datasets = query.split(' ')
                if ',' in test_for_two_datasets[3]: # join command
                    s.sendall(f"maple,join_maple.py,{query}".encode())
                else: # filter
                    s.sendall(f"maple,filter_maple.py,{query}".encode())
                    while True:
                        if s.recv(1024).decode() == "finished":
                            break
                    print("DONE")
                    # s.sendall(f"juice,filter_juice.py,{query}".encode())
                # MAKE SURE FINISHED (ex: response from leader)
               # s.sendall((f"juice,SQLjuice.py,{num_vms},SQL_intermediate_query_prefix,SQL_query_destination,1,range").encode('utf-8'))
            elif command == "append":
                s.connect(leader)

                s.sendall((f"append,{sdfs_fname},{additions}").encode())
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
                print("LEADER LIST:", self.leader_list)
                print()
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
            elif len(user_input) >= 5 and user_input[0:5] == "maple":
                inp = user_input.split(" ", 6)
                if len(inp) < 5:
                    print("MAPLE command following format: [maple_exe] [num_maples] [sdfs_intermediate_filename_prefix] [sdfs_src_directory].")
                    continue
                field = None
                if len(inp) == 6:
                    _, maple_exe, num_maples, sdfs_intermediate_filename_prefix, sdfs_src_directory, field = inp
                    self.command_sender(command = "maple", maple_exe=maple_exe, num_maples=num_maples, sdfs_intermediate_filename_prefix=sdfs_intermediate_filename_prefix, sdfs_src_directory=sdfs_src_directory, field=field)
                else:
                    _, maple_exe, num_maples, sdfs_intermediate_filename_prefix, sdfs_src_directory = inp
                    self.command_sender(command = "maple", maple_exe=maple_exe, num_maples=num_maples, sdfs_intermediate_filename_prefix=sdfs_intermediate_filename_prefix, sdfs_src_directory=sdfs_src_directory)
                    

                

            elif len(user_input) >= 5 and user_input[0:5] == "juice":
                inp = user_input.split(" ")
                if len(inp) < 7:
                    print("JUICE command following format: [juice_exe] [num_juices] [sdfs_intermediate_filename_prefix] [sdfs_dest_filename] [delete_input = {0,1}] [partition_method].")
                    continue
                _, juice_exe, num_juices, sdfs_intermediate_filename_prefix, sdfs_dest_filename, delete_input, partition_method = inp
            
                self.command_sender(command = "juice", juice_exe=juice_exe, num_juices=num_juices, sdfs_intermediate_filename_prefix=sdfs_intermediate_filename_prefix, sdfs_dest_filename=sdfs_dest_filename, delete_input=delete_input, partition_method=partition_method)
            elif len(user_input) >= 6 and user_input[0:6].lower() == "select":
                self.command_sender(command = "select", query=user_input)
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
            # for member_id, member_info in self.MembershipList.items():
            #     sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            #     print(" send list to:", (member_info['addr'][0],LIST_PORT))
            #     sock.connect((member_info['addr'][0],LIST_PORT))

            #     #serialize leader list dictionary
            #     json_str = json.dumps(self.leader_list)
            #     sock.sendall(f"list,{json_str}".encode('utf-8'))
            print(sdfs_fname)
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

        port_file_thread = threading.Thread(target=self.file_port)
        port_file_thread.daemon = True
        port_file_thread.start()

        leader_thread_list = threading.Thread(target=self.leader_list_server)
        leader_thread_list.daemon = True
        leader_thread_list.start()

        maple_thread = threading.Thread(target=self.maple_func)
        maple_thread.daemon = True
        maple_thread.start()
        
        juice_thread = threading.Thread(target=self.juice_func)
        juice_thread.daemon = True
        juice_thread.start()

        receiver_thread.join()
        sender_thread.join()
        user_thread.join()
        leader_thread_list.join()
        file_thread.join()
        maple_thread.join()
        juice_thread.join()
        port_file_thread.join()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--protocol-period', type=float, help='Protocol period T in seconds', default=0.25)
    parser.add_argument('-d', '--drop-rate', type=float,
                        help='The message drop rate',
                        default=0)
    args = parser.parse_args()
    
    server = Server(args)
    server.run()
