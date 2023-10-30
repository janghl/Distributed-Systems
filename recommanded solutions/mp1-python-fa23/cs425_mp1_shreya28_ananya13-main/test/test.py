#importing modules
import socket
import os
import random
import pickle

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

machine1_log = [
    "Hi, my name is ananya", 
    "Hi my name is shreya", 
    "CS 425 mp!", 
    "I love dogs and cats"
]

#list = []
def log_files():
    list = [
        "Welcome to CS425",
        "I like MapReduce"
    ]
    for i in range (20):
        if random.choice([True, False]):
            list.append(random.choice(machine1_log))
        else:
            list.append("Random log line")
    return list

m1 = log_files()
m2 = log_files()
m3 = log_files()
m4 = log_files()
m5 = log_files()
m6 = log_files()
m7 = log_files()
m8 = log_files()
m9 = log_files()
m10 = log_files()


g1 = "grep -c icecream " + "test_case.log" #0
g2 = "grep -n Welcome " + "test_case.log" #0
g3 = "grep -c Welcome " + "test_case.log" #1
g4 = "grep -c Hi " + "test_case.log" #<20
g5 = "grep -v Random " + "test_case.log" 

grep_list = [g1,g2,g3,g4,g5]

m1.extend(grep_list)
m2.extend(grep_list)
m3.extend(grep_list)
m4.extend(grep_list)
m5.extend(grep_list)
m6.extend(grep_list)
m7.extend(grep_list)
m8.extend(grep_list)
m9.extend(grep_list)
m10.extend(grep_list)


client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_1, 12345))
client_socket.send(pickle.dumps(m1))
response1 = pickle.loads(client_socket.recv(1024))
print("response from server 1")
print(response1)
client_socket.close()

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_2, 12345))
client_socket.send(pickle.dumps(m2))
response2 = pickle.loads(client_socket.recv(1024))
print("response from server 2")
print(response2)
client_socket.close()

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_3, 12345))
client_socket.send(pickle.dumps(m3))
response3 = pickle.loads(client_socket.recv(1024))
print("response from server 3")
print(response3)
client_socket.close()

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_4, 12345))
client_socket.send(pickle.dumps(m4))
response4 = pickle.loads(client_socket.recv(1024))
print("response from server 4")
print(response4)
client_socket.close()

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_5, 12345))
client_socket.send(pickle.dumps(m5))
response5 =pickle.loads(client_socket.recv(1024))
print("response from server 5")
print(response5)
client_socket.close()

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_6, 12345))
client_socket.send(pickle.dumps(m6))
response6 = pickle.loads(client_socket.recv(1024))
print("response from server 6")
print(response6)
client_socket.close()

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_7, 12345))
client_socket.send(pickle.dumps(m7))
response7 = pickle.loads(client_socket.recv(1024))
print("response from server 7")
print(response7)
client_socket.close()

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_8, 12345))
client_socket.send(pickle.dumps(m8))
response8 = pickle.loads(client_socket.recv(1024))
print("response from server 8")
print(response1)
client_socket.close()

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_9, 12345))
client_socket.send(pickle.dumps(m9))
response9 = pickle.loads(client_socket.recv(1024))
print("response from server 9")
print(response9)
client_socket.close()

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client_socket.connect((ip_10, 12345))
client_socket.send(pickle.dumps(m10))
response10 = pickle.loads(client_socket.recv(1024))
print("response from server 10")
print(response10)
client_socket.close()


