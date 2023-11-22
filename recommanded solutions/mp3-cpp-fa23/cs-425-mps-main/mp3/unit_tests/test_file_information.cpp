#include <iostream>
#include <cassert>
#include <vector>
#include "../name_node/file_information.h"

const int NUM_REPLICAS = 2;
const int X = 1; //majority of ACK in writing

//initial File information
extern const int NUM_REPLICAS;
void test_initial_file() {
    FileInformation fileInfo("testfile.txt");
    assert(fileInfo.get_file_name() ==  "testfile.txt");
    assert(fileInfo.get_update_counter() == 0);  // default is 0
    assert(!fileInfo.get_replica_ids().empty());
    assert(fileInfo.get_replica_ids().size() == NUM_REPLICAS);
}

//test 5 reads and 5 write for 2 file
void test_enqueue_10_reads_10_writes_2_file(){
    std::string filename_1 = "test_file_1.txt";
    std::string filename_2 = "test_file_1.txt";
    FileInformation fileInfo_1(filename_1);
    FileInformation fileInfo_2(filename_2);

    std::vector<Request> req_vector_1;
    std::vector<Request> req_vector_2;
    for(int i = 0; i < 10; i ++){
        Request r_1(OperationType::READ, filename_1);
        Request w_2(OperationType::WRITE, filename_2,nullptr,0,1);
        req_vector_1.push_back(r_1);
        req_vector_2.push_back(w_2);
        
    }
    for(int i = 0; i < 10; i ++){
        Request w_1(OperationType::WRITE, filename_1,nullptr,0,1);
        Request r_2(OperationType::READ, filename_2);
        req_vector_1.push_back(w_1);
        req_vector_2.push_back(r_2);
    }
    
    
    
    for (int i = 0; i < req_vector_1.size(); ++i) {
        fileInfo_1.add_request_to_queue(req_vector_1[i]); 
    }
    for (int i = 0; i < req_vector_2.size(); ++i) {
        fileInfo_2.add_request_to_queue(req_vector_2[i]); 
    }

    std::vector<OperationType> expectedOrder_1 = {
        OperationType::READ, OperationType::READ, OperationType::READ, OperationType::READ,
        OperationType::WRITE, 
        OperationType::READ,OperationType::READ, OperationType::READ,OperationType::READ,
        OperationType::WRITE, 
        OperationType::READ,OperationType::READ,
        OperationType::WRITE, OperationType::WRITE, OperationType::WRITE, OperationType::WRITE,
        OperationType::WRITE, OperationType::WRITE, OperationType::WRITE, OperationType::WRITE
    };
    std::vector<OperationType> expectedOrder_2 = {
        OperationType::WRITE, OperationType::WRITE, OperationType::WRITE, OperationType::WRITE,
        OperationType::READ, 
        OperationType::WRITE, OperationType::WRITE, OperationType::WRITE, OperationType::WRITE,
        OperationType::READ, 
        OperationType::WRITE, OperationType::WRITE,
        OperationType::READ,OperationType::READ, OperationType::READ, OperationType::READ,
        OperationType::READ,OperationType::READ, OperationType::READ, OperationType::READ
        
    };

    for (const auto& op : expectedOrder_1) {
        Request req1 = fileInfo_1.pop_request_from_queue(); 
        //std::cout<<(req1.get_request_type() == OperationType::WRITE); 
        assert(req1.get_request_type() == op);
    }

    for (const auto& op : expectedOrder_2) {
        Request req2 = fileInfo_2.pop_request_from_queue();  
        //std::cout<<(req2.get_request_type() == OperationType::WRITE);
        assert(req2.get_request_type() == op);
    }
}



//test 5 reads and 1 write
void test_enqueue_5_reads_1_writes(){
    std::string filename = "test_file_1.txt";
    FileInformation fileInfo(filename);
    
    std::vector<Request> req_vector;
    for(int i = 0; i < 5; i ++){
        //read request
        Request r(OperationType::READ, filename);
        req_vector.push_back(r);
    }
    
    Request w(OperationType::WRITE, filename,nullptr,0,1);
    req_vector.push_back(w);
    
    
    for (int i = 0; i < req_vector.size(); ++i) {
        fileInfo.add_request_to_queue(req_vector[i]); 
    }

    std::vector<OperationType> expectedOrder = {
        OperationType::READ, OperationType::READ, OperationType::READ, OperationType::READ,
        OperationType::WRITE, OperationType::READ
    };

    for (const auto& op : expectedOrder) {
        Request req = fileInfo.pop_request_from_queue();  
        assert(req.get_request_type() == op);
    }
}
// Enqueue 5 reads and then 5 writes.
void test_enqueue_5_reads_5_writes(){
    std::string filename = "test_file_2.txt";
    FileInformation fileInfo(filename);
    
    std::vector<Request> req_vector;
    for(int i = 0; i < 5; i ++){
        //read request
        Request r(OperationType::READ, filename);
        req_vector.push_back(r);
    }
    
    for(int i = 0; i < 5; i ++){
        //wrtie request
        Request w(OperationType::WRITE, filename,nullptr,0,i);
        req_vector.push_back(w);
    }
    
    for (int i = 0; i < req_vector.size(); ++i) {
        fileInfo.add_request_to_queue(req_vector[i]); 
    }

    std::vector<OperationType> expectedOrder = {
        OperationType::READ, OperationType::READ, OperationType::READ, OperationType::READ,
        OperationType::WRITE, OperationType::READ,
        OperationType::WRITE, OperationType::WRITE, OperationType::WRITE, OperationType::WRITE
    };

    for (const auto& op : expectedOrder) {
        Request req = fileInfo.pop_request_from_queue();  
        assert(req.get_request_type() == op);
    }
}

int main() {
    test_initial_file();
    test_enqueue_10_reads_10_writes_2_file();
    test_enqueue_5_reads_1_writes();
    test_enqueue_5_reads_5_writes();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}