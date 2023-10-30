#importing modules
import socket
import os
import subprocess
import shlex
import threading

server1_name = "fa23-cs425-6601.cs.illinois.edu"
ip_1 = socket.gethostbyname(server1_name)

server2_name = "fa23-cs425-6602.cs.illinois.edu"
ip_2 = socket.gethostbyname(server2_name)

server3_name = "fa23-cs425-6603.cs.illinois.edu"
ip_3 = socket.gethostbyname(server3_name)

server4_name = "fa23-cs425-6604.cs.illinois.edu"
ip_4 = socket.gethostbyname(server4_name)

server5_name = "fa23-cs425-6605.cs.illinois.edu"
ip_5 = socket.gethostbyname(server5_name)

server6_name = "fa23-cs425-6606.cs.illinois.edu"
ip_6 = socket.gethostbyname(server6_name)

server7_name = "fa23-cs425-6607.cs.illinois.edu"
ip_7 = socket.gethostbyname(server7_name)

server8_name = "fa23-cs425-6608.cs.illinois.edu"
ip_8 = socket.gethostbyname(server8_name)

server9_name = "fa23-cs425-6609.cs.illinois.edu"
ip_9 = socket.gethostbyname(server9_name)

server10_name = "fa23-cs425-6610.cs.illinois.edu"
ip_10 = socket.gethostbyname(server10_name)

#servers' list & details
server_details = [
    (ip_1,12345,"machine.i.log"),
    (ip_2,12345,"machine.i.log"),
    (ip_3,12345,"machine.i.log"),
    (ip_4,12345,"machine.i.log"),
   (ip_5,12345,"machine.i.log"),
    (ip_6,12345,"machine.i.log"),
    (ip_7,12345,"machine.i.log"),
    (ip_8,12345,"machine.i.log"),
   (ip_9,12345,"machine.i.log"),
    (ip_10,12345,"machine.i.log")
]

#function to receive response from user
def get_response(ip, port, filename_fetched,number):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_socket.connect((ip, port))
        print("Connected to VM ", number)

    except:
        print("error connecting to VM ", number);	print()
        return

    grep_command = "grep -c \([0-9]\{1,3\}\.\)\{3\}[0-9]\{1,3\} " + filename_fetched
    client_socket.send(grep_command.encode())
    

    #receiving output
    response = client_socket.recv(1024)
    print("displaying output")
    print("VM", number, " : ",response.decode())

    client_socket.close()


#sending request to servers from server list
outside_vms = [1,2,3,4,5,6,7,8,9,10]
i = 0
for ip, port, filename_fetched in server_details:
    
    get_response(ip, port, filename_fetched,outside_vms[i]); i+=1
 #   print("success fetching from server.... moving on")
#    print()
