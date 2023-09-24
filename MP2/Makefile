CXX = g++
CXXFLAGS = -pthread -std=c++17
DEBUGFLAGS = -g -Wall -Werror -O0 $(CXXFLAGS)

gossip: gossip.cpp
	$(CXX) $(CXXFLAGS) -O3 gossip.cpp -o gossip.out

gossip_debug: gossip.cpp
	$(CXX) $(DEBUGFLAGS) gossip.cpp -o gossip_debug.out

clean: 
	rm *.log *.temp