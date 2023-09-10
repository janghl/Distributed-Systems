CXX = g++
CXXFLAGS = -std=c++17 -pthread
DEBUGFLAGS = -g -Wall -Werror -O0 $(CXXFLAGS)

project: server client

project_debug: server_debug client_debug

server: server.cpp
	$(CXX) $(CXXFLAGS) -O3 server.cpp -o server.out

server_debug: server.cpp
	$(CXX) $(DEBUGFLAGS) server.cpp -o server_debug.out

client: client_main.cpp
	$(CXX) $(CXXFLAGS) -O3 client_main.cpp -o client.out

client_debug: client_main.cpp 
	$(CXX) $(DEBUGFLAGS) client_main.cpp -o client_debug.out

clean:
	rm client server client_debug server_debug
