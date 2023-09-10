CXX = g++
CXXFLAGS = -std=c++17 -pthread
DEBUGFLAGS = -g -Wall -Werror -O0 $(CXXFLAGS)

project: server client

project_debug: server_debug client_debug

server: server.cpp
	$(CXX) $(CXXFLAGS) -O3 server.cpp -lstdc++fs -o server.out

server_debug: server.cpp
	$(CXX) $(DEBUGFLAGS) server.cpp -lstdc++fs -o server_debug.out

client: client_main.cpp
	$(CXX) $(CXXFLAGS) -O3 client_main.cpp -o client.out

client_debug: client_main.cpp 
	$(CXX) $(DEBUGFLAGS) client_main.cpp -o client_debug.out

behave_debug: behave.cpp
	$(CXX) $(DEBUGFLAGS) behave.cpp -o behave_debug.out

behave: behave.cpp
	$(CXX) $(CXXFLAGS) behave.cpp -o behave.out

clean:
	rm *.out *.log
