import subprocess
import shlex
import os
import threading
import socket
import pickle

server_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

port = 12345
DNS_name = "fa23-cs425-6601.cs.illinois.edu"
ip = socket.gethostbyname(DNS_name)


server_socket.bind((ip,port))
print(f"Server details are as follows: The ip address is: {ip} and the port is :{port}")

server_socket.listen(10)

client_socket,client_address = server_socket.accept()
print(f"Connection established from {client_address}")

data = client_socket.recv(1024)
input_list = pickle.loads(data)
print(input_list)
print()

grep_commands = input_list[-5:]
print(grep_commands)
print()

file_data = input_list[:-5]
print(file_data)
print()

with open("test_case.log", 'w') as f:
    for i in file_data:
        f.write(i+ '\n')

with open("test_case.log",'r') as f:
	output = []
	for i in grep_commands:
		grep_command = i.split()
		print(grep_command)
		response = subprocess.run(grep_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		result = response.stdout.decode()
		output.append(result)

client_socket.send(pickle.dumps(output))
client_socket.close()
