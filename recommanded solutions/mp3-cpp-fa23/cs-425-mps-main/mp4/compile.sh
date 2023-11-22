# To compile the regular program without tests
<<'##REGULAR_PROGRAM'
g++ -std=c++17 -g2 										\
	-DBLOCKREPORT										\
	-DFAILED_PRINT										\
	$(ls *.cpp | grep -v -E 'test|setup')							\
	./worker/*.cpp										\
	-I ./											\
	-I ./worker										\
												\
	$(ls ../mp3/*.cpp | grep -v -E 'test|setup|main') 					\
	../mp3/data_node/*.cpp									\
	../mp3/name_node/*.cpp									\
	../mp3/miscellaneous/*.cpp								\
	-I ../mp3/data_node									\
	-I ../mp3/name_node									\
	-I ../mp3/miscellaneous									\
	-I ../mp3										\
												\
	$(ls ../mp2/*.cpp | grep -v -E 'test|main|setup')					\
	$(ls ../mp1/*.cpp | grep -v -E 'test|main|setup')					\
	-I ../mp1 -I ../mp2									\
												\
	-lpthread -lstdc++fs									\
	-DTCP											\
	-o maple_juice										\
##REGULAR_PROGRAM

# To compile test_job.cpp
<<'##TEST_JOB'
g++ -std=c++17 -g2 										\
	-DBLOCKREPORT										\
	./unit_tests/test_job.cpp								\
	../mp1/utils.cpp									\
	../mp2/utils.cpp									\
	../mp3/utils.cpp									\
	-I ./											\
	-I ../mp1										\
	-I ../mp2										\
	-I ../mp3										\
	-I ../mp3/data_node									\
	-o test_job										\
##TEST_JOB

# To compile test_task.cpp
<<'##TEST_TASK'
g++ -std=c++17 -g2 										\
	-DBLOCKREPORT										\
	./unit_tests/test_task.cpp								\
	../mp1/utils.cpp									\
	../mp2/utils.cpp									\
	../mp3/utils.cpp									\
	-I ./											\
	-I ../mp1										\
	-I ../mp2										\
	-I ../mp3										\
	-I ../mp3/data_node									\
	-o test_task										\
##TEST_TASK

# To compile test_worker_server.cpp
#<<'##TEST_WORKER_SERVER'
g++ -std=c++17 -g2										\
	-DBLOCKREPORT                                                                           \
	-DDEBUG_MP4_WORKER_SERVER								\
        ./unit_tests/test_worker_server.cpp                                                     \
	./worker/worker_server.cpp								\
	$(ls *.cpp | grep -v -E 'main|test|setup')                                              \
        ../mp1/utils.cpp                                                                        \
        ../mp2/utils.cpp                                                                        \
        ../mp3/utils.cpp                                                                        \
	../mp3/data_node/data_node_client.cpp							\
	../mp3/operation_type.cpp								\
        -I ./                                                                                   \
	-I ./worker										\
        -I ../mp1                                                                               \
        -I ../mp2                                                                               \
        -I ../mp3                                                                               \
        -I ../mp3/data_node                                                                     \
	-lpthread -lstdc++fs                                                                    \
        -o test_worker_server
g++ -std=c++17 -g2 ./unit_tests/maple_test_worker_server.cpp -o ./unit_tests/maple_test_worker_server.o
g++ -std=c++17 -g2 ./unit_tests/juice_test_worker_server.cpp -o ./unit_tests/juice_test_worker_server.o
##TEST_WORKER_SERVER
