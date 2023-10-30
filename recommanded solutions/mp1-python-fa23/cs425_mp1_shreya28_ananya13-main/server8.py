#importing necessary modules required
import subprocess
import shlex
import os
import threading
import socket


#initializing server details
server_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

port = 12345
DNS_name = "fa23-cs425-6608.cs.illinois.edu"
ip = socket.gethostbyname(DNS_name)

server_socket.bind((ip,port))
print(f"Server details are as follows: The ip address is: {ip} and the port is :{port}")

server_socket.listen(10)


#function to handle client requests and respond back 
def get_request(client_socket):      
    try: 
        input = client_socket.recv(1024).decode()
        grep_list = input.split()
        filename_fetched = grep_list[-1]
        print(grep_list)
        print(filename_fetched)
        with open(filename_fetched,'rb') as file:

            data_read = file.read(1024) # data read from file
            
            
         

            try:
                #python library implementation to run grep commands through python
                response = subprocess.run(grep_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                if response.returncode == 0:
                    final_result = response.stdout
                    client_socket.send(final_result)
                    
                else:
                    error_result = "Error: not able to find suitable value"
                    client_socket.send(error_result.encode())
                    
            except Exception as e:
                except_error = "Grep Error"
                print(e)
                client_socket.send(except_error.encode())

    finally:
        client_socket.close()

#setting up loop to accept client requests
while True:
    client_socket,client_address = server_socket.accept()
    print(f"Connection established from {client_address}")


    thread = threading.Thread(target= get_request, args=(client_socket,))
    thread.start()
