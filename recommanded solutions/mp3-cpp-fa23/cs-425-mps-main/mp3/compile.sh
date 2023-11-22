# To compile the regular program without tests
# <<'##REGULAR_PROGRAM'
g++ -std=c++17 -g2 										\
	-DBLOCKREPORT										\
	-DFAILED_PRINT										\
	$(ls *.cpp | grep -v -E 'test|setup') 				\
	./data_node/*.cpp									\
	./name_node/*.cpp									\
	./miscellaneous/*.cpp								\
	-I ./data_node										\
	-I ./name_node										\
	-I ./miscellaneous									\
	-I ./												\
	$(ls ../mp2/*.cpp | grep -v -E 'test|main|setup')	\
	$(ls ../mp1/*.cpp | grep -v -E 'test|main|setup')	\
	-I ../mp1 -I ../mp2									\
	-lpthread -lstdc++fs								\
	-DTCP												\
	-o sdfs												\
#	-DDEBUG_MP3_MAIN
#	-DDEBUG_MP3_NAME_NODE_BLOCK_REPORT					\
#	-DDEBUG_MP3_REQUEST_QUEUES							\
#	-DDEBUG_MP3											\
#	-DDEBUG_MP3_DATA_NODE_SERVER						\
#	-DDEBUG_MP3_FILE_INFORMATION						\
#	-DDEBUG_MP3_DATA_NODE_FILE_MANAGER							\
#	-DPURGED_PRINT

##REGULAR_PROGRAM

# To compile test_block_report_manager.cpp
#g++ -std=c++17 -g2 ./unit_tests/test_block_report_manager.cpp -I ./data_node -I ./ -o test_block_report -lstdc++fs

# To compile test_membership_list_manager.cpp
#g++ -std=c++17 -g2 ./unit_tests/test_membership_list_manager.cpp ../mp2/utils.cpp ../mp1/utils.cpp -I ../mp1 -I ../mp2 -I ./data_node -o test_membership_list -lstdc++fs -DBLOCKREPORT

# To compile test_request.cpp
#g++ -std=c++17 -g2 ./unit_tests/test_request.cpp utils.cpp ../mp1/utils.cpp -I ./ -I ../mp1 -o test_request

# To compile test_response.cpp
<<'##TEST_RESPONSE'
g++ -std=c++17 -g2																	\
	-DBLOCKREPORT                                                                   \
	$(ls *.cpp | grep -v -E 'test|main') 											\
	$(ls ./unit_tests/test_response.cpp)                                    		\
	./data_node/*.cpp                                                               \
	-I ./data_node                                                                  \
	-I ./                                                                           \
	$(ls ../mp2/*.cpp | grep -v -E 'test|main|setup')                               \
	$(ls ../mp1/*.cpp | grep -v -E 'test|main|setup')                               \
	-I ../mp1 -I ../mp2                                                             \
	-lpthread -lstdc++fs                                                            \
	-DUDP                                                                           \
	-o test_response
##TEST_RESPONSE

# To compile test_data_node_server.cpp
<<'##TEST_DN_SERVER'
g++ -std=c++17 -g2																	\
	-DBLOCKREPORT                                                                   \
	$(ls *.cpp | grep -v -E 'test|main') 											\
	$(ls ./unit_tests/test_data_node_server.cpp)                                    \
	./data_node/*.cpp                                                               \
	-I ./data_node                                                                  \
	-I ./                                                                           \
	$(ls ../mp2/*.cpp | grep -v -E 'test|main|setup')                               \
	$(ls ../mp1/*.cpp | grep -v -E 'test|main|setup')                               \
	-I ../mp1 -I ../mp2                                                             \
	-lpthread -lstdc++fs                                                            \
	-DUDP                                                                           \
	-o test_data_node_server
##TEST_DN_SERVER

# To compile test_data_node_client.cpp
<<'##TEST_DN_ETOE'
g++ -std=c++17 -g2                                                                  \
	-DBLOCKREPORT                                                                   \
	$(ls *.cpp | grep -v -E 'test|main|setup')                                      \
	$(ls ./unit_tests/test_data_node_client.cpp)                                    \
	./data_node/*.cpp                                                               \
	-I ./data_node                                                                  \
	-I ./                                                                           \
	$(ls ../mp2/*.cpp | grep -v -E 'test|main|setup')                               \
	$(ls ../mp1/*.cpp | grep -v -E 'test|main|setup')                               \
	-I ../mp1 -I ../mp2                                                             \
	-lpthread -lstdc++fs                                                            \
	-DUDP                                                                           \
	-DDEBUG_MP3_DATA_NODE_SERVER													\
	-DDEBUG_MP3																		\
	-o test_data_node_client
##TEST_DN_ETOE

# To compile test_name_node_read_repair.cpp
<<'##TEST_READ_REPAIR'
g++ -std=c++17 -g2                                                                  \
	-DBLOCKREPORT                                                                   \
	$(ls *.cpp | grep -v -E 'test|main')                                            \
	$(ls ./unit_tests/test_name_node_read_repair.cpp)                               \
	./data_node/*.cpp                                                               \
	./name_node/*.cpp																\
	-I ./data_node                                                                  \
	-I ./name_node                                                                  \
	-I ./                                                                           \
	$(ls ../mp2/*.cpp | grep -v -E 'test|main|setup')                               \
	$(ls ../mp1/*.cpp | grep -v -E 'test|main|setup')                               \
	-I ../mp1 -I ../mp2                                                             \
	-lpthread -lstdc++fs                                                            \
	-DUDP                                                                           \
	-o test_name_node_read_repair
##TEST_READ_REPAIR

