CXX = g++
CXXFLAGS = -std=c++17 -pthread
DEBUGFLAGS = -g -Wall -Werror -O0 $(CXXFLAGS)

gossip: gossip.cpp
	$(CXX) $(CXXFLAGS) -O3 gossip.cpp -lstdc++fs -o gossip.out

server_debug: gossip.cpp
	$(CXX) $(DEBUGFLAGS) gossip.cpp -lstdc++fs -o gossip_debug.out

clean:
	rm *.out machine*.log *.temp
