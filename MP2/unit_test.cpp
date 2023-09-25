#include "sender.h"

void test1() {
  Controller controller;
  controller.controller();
  controller.self_node_.host="self";
  controller.self_node_.port="8080";
  controller.self_node_.time_stamp=10;
  Controller::MembershipEntry self_entry;
  self_entry.local_time=1025;
  self_entry.count = 1024;
  self_entry.status = Controller::Status::kAlive;
  controller.membership_list_[controller.self_node_] = self_entry;
  // specify fields for list1
  std::map<Controller::NodeId, Controller::MembershipEntry> list2;
  // specify fields for list2
  std::map<Controller::NodeId, Controller::MembershipEntry> expected;
  Controller::NodeId node2;
  Controller::NodeId node3;
  Controller::NodeId node4;
  Controller::NodeId node5;
  Controller::NodeId node6;
  Controller::NodeId node7;
  Controller::NodeId node8;
  Controller::NodeId node9;
  Controller::MembershipEntry entry2;
  Controller::MembershipEntry entry3;
  Controller::MembershipEntry entry4;
  Controller::MembershipEntry entry5;
  Controller::MembershipEntry entry6;
  Controller::MembershipEntry entry7;
  Controller::MembershipEntry entry8;
  Controller::MembershipEntry entry9;
  controller.using_suspicion_ = false;
  node2.host = "fa23-cs425-6902.cs.illinois.edu";
  node2.port = "8080";
  node2.time_stamp = 10;
  node3.host = "fa23-cs425-6903.cs.illinois.edu";
  node3.port = "8080";
  node3.time_stamp = 10;
  node4.host = "fa23-cs425-6901.cs.illinois.edu";
  node4.port = "8080";
  node4.time_stamp = 10;
  node5.host = "fa23-cs425-6902.cs.illinois.edu";
  node5.port = "8080";
  node5.time_stamp = 10;
  node6.host = "fa23-cs425-6903.cs.illinois.edu";
  node6.port = "8080";
  node6.time_stamp = 10;
  node7.host = "fa23-cs425-6901.cs.illinois.edu";
  node7.port = "8080";
  node7.time_stamp = 10;
  node8.host = "fa23-cs425-6902.cs.illinois.edu";
  node8.port = "8080";
  node8.time_stamp = 10;
  node9.host = "fa23-cs425-6903.cs.illinois.edu";
  node9.port = "8080";
  node9.time_stamp = 10;
  entry4.count = 1024;
  entry4.local_time = 1025;
  entry4.status = Controller::Status::kAlive;
  entry7.count = 1024;
  entry7.local_time = 1025;
  entry7.status = Controller::Status::kAlive;
  entry5.status = Controller::Status::kFailed;
  entry8.status = Controller::Status::kFailed;
  entry2.status = Controller::Status::kAlive;
  entry5.count = 1024;
  entry5.local_time = 1025;
  entry2.count = 995;
  entry2.local_time = 246;
  entry8.count = 995;
  entry8.local_time = 246;
  entry6.status = Controller::Status::kAlive;
  entry9.status = Controller::Status::kAlive;
  entry3.status = Controller::Status::kAlive;
  entry6.count = 1024;
  entry6.local_time = 1025;
  entry3.count = 995;
  entry3.local_time = 246;
  entry9.count = 1024;
  entry9.local_time = 1025;

  controller.membership_list_[node2] = entry2;
  controller.membership_list_[node3] = entry3;
  list2[node4] = entry4;
  list2[node5] = entry5;
  list2[node6] = entry6;
  expected[node7] = entry7;
  expected[node8] = entry8;
  expected[node9] = entry9;
  controller.Merge(list2);
  // specify fields for expected

// for(auto i:expected){

//   std::cout<<i.first.host<<i.first.port<<i.first.time_stamp<<"\n";
//   std::cout<<i.second.local_time<<i.second.count<<controller.status_to_string_[i.second.status]<<"\n";
// }
  controller.membership_list_.erase(controller.self_node_);
  if(controller.membership_list_ == expected)
       std::cout<<"true";
  else
   std::cout<<"false";
}
void test2() {
  Controller controller;
  controller.controller();
  controller.self_node_.host="self";
  controller.self_node_.port="8080";
  controller.self_node_.time_stamp=10;
  Controller::MembershipEntry self_entry;
  self_entry.local_time=1025;
  self_entry.count = 1024;
  self_entry.status = Controller::Status::kAlive;
  controller.membership_list_[controller.self_node_] = self_entry;
  // specify fields for list1
  std::map<Controller::NodeId, Controller::MembershipEntry> list2;
  // specify fields for list2
  std::map<Controller::NodeId, Controller::MembershipEntry> expected;
  Controller::NodeId node1;
  Controller::NodeId node2;
  Controller::NodeId node3;
  Controller::NodeId node4;
  Controller::NodeId node5;
  Controller::NodeId node6;
  Controller::NodeId node7;
  Controller::NodeId node8;
  Controller::NodeId node9;
  Controller::MembershipEntry entry1;
  Controller::MembershipEntry entry2;
  Controller::MembershipEntry entry3;
  Controller::MembershipEntry entry4;
  Controller::MembershipEntry entry5;
  Controller::MembershipEntry entry6;
  Controller::MembershipEntry entry7;
  Controller::MembershipEntry entry8;
  Controller::MembershipEntry entry9;
  node1.host = "fa23-cs425-6901.cs.illinois.edu";
  node1.port = "8080";
  node1.time_stamp = 10;
  node2.host = "fa23-cs425-6902.cs.illinois.edu";
  node2.port = "8080";
  node2.time_stamp = 10;
  node3.host = "fa23-cs425-6903.cs.illinois.edu";
  node3.port = "8080";
  node3.time_stamp = 10;
  node4.host = "fa23-cs425-6901.cs.illinois.edu";
  node4.port = "8080";
  node4.time_stamp = 10;
  node5.host = "fa23-cs425-6902.cs.illinois.edu";
  node5.port = "8080";
  node5.time_stamp = 10;
  node6.host = "fa23-cs425-6903.cs.illinois.edu";
  node6.port = "8080";
  node6.time_stamp = 10;
  node7.host = "fa23-cs425-6901.cs.illinois.edu";
  node7.port = "8080";
  node7.time_stamp = 10;
  node8.host = "fa23-cs425-6902.cs.illinois.edu";
  node8.port = "8080";
  node8.time_stamp = 10;
  node9.host = "fa23-cs425-6903.cs.illinois.edu";
  node9.port = "8080";
  node9.time_stamp = 10;
  
  controller.using_suspicion_ = true;
  entry4.status = Controller::Status::kFailed;
  entry7.status = Controller::Status::kFailed;
  entry1.status = Controller::Status::kFailed;
  entry4.count = 1024;
  entry4.local_time = 1025;
  entry1.count = 995;
  entry1.local_time = 246;
  entry7.count = 995;
  entry7.local_time = 246;

  entry5.status = Controller::Status::kFailed;
  entry8.status = Controller::Status::kFailed;
  entry2.status = Controller::Status::kAlive;
  entry5.count = 1024;
  entry5.local_time = 1025;
  entry2.count = 995;
  entry2.local_time = 246;
  entry8.count = 995;
  entry8.local_time = 246;

  entry6.status = Controller::Status::kSuspected;
  entry9.status = Controller::Status::kSuspected;
  entry3.status = Controller::Status::kAlive;
  entry6.count = 1024;
  entry6.local_time = 1025;
  entry3.count = 995;
  entry3.local_time = 246;
  entry9.count = 1024;
  entry9.local_time = 1025;
  // Fix this
  controller.membership_list_[node1] = entry1;
  controller.membership_list_[node2] = entry2;
  controller.membership_list_[node3] = entry3;
  list2[node4] = entry4;
  list2[node5] = entry5;
  list2[node6] = entry6;
  expected[node7] = entry7;
  expected[node8] = entry8;
  expected[node9] = entry9;

  controller.Merge(list2);
  controller.membership_list_.erase(controller.self_node_);
  // specify fields for expected
  if(controller.membership_list_ == expected)
       std::cout<<"true";
  else
   std::cout<<"false";
}





void checkertest1() {
  Controller controller; // machine=0, num=1, expect=2Controller controller;
  controller.controller();
  controller.self_node_.host="self";
  controller.self_node_.port="8080";
  controller.self_node_.time_stamp=10;
  Controller::MembershipEntry self_entry;
  self_entry.local_time=1025;
  self_entry.count = 1024;
  self_entry.status = Controller::Status::kAlive;
  controller.membership_list_[controller.self_node_] = self_entry;
  std::map<Controller::NodeId, Controller::MembershipEntry> list;
  Controller::NodeId node1;
  controller.using_suspicion_ = false;
  Controller::MembershipEntry entry1;

  node1.host="node1";
  node1.port="8080";
  node1.time_stamp=10;
  entry1.count = 1044; // machine
  entry1.local_time = 975;
  entry1.status = Controller::Status::kAlive;
  // Fix this
  controller.membership_list_[node1] = entry1;
  controller.membership_list_[controller.self_node_] = self_entry;
  controller.Checker();
  if(controller.membership_list_.find(node1) == controller.membership_list_.end())
       std::cout<<"true";
  else
   std::cout<<"false";
  // make suspicion var public
}
void checkertest3() {
// fix this with public 
  Controller controller;
  controller.controller();
  controller.self_node_.host="self";
  controller.using_suspicion_ = true;
  std::map<Controller::NodeId, Controller::MembershipEntry> list;
  Controller::NodeId node1;
  Controller::NodeId node2;
  Controller::NodeId node3;
  Controller::NodeId node4;
  Controller::MembershipEntry entry1;
  Controller::MembershipEntry entry2;
  Controller::MembershipEntry entry3;
  Controller::MembershipEntry entry4;

  entry1.count = 1044; // machine
  entry1.local_time = 1094;
  entry1.status = Controller::Status::kAlive;
  entry2.count = 1014;
  entry2.local_time = 1064;
  entry2.status = Controller::Status::kAlive;
  entry3.count = 1014;
  entry3.local_time = 1044;
  entry3.status = Controller::Status::kAlive;
  entry4.count = 1014;
  entry4.local_time = 1024;
  entry4.status = Controller::Status::kAlive;
  // make stuff public to fix this
  controller.membership_list_[node1] = entry1;
  controller.membership_list_[node2] = entry2;
  controller.membership_list_[node3] = entry3;
  controller.membership_list_[node4] = entry4;
  controller.Checker();
  if(controller.membership_list_.find(node1) == controller.membership_list_.end())
       std::cout<<"true";
  else
   std::cout<<"false";
}
void checkertest2() {
  Controller controller;
  controller.controller();
  controller.self_node_.host="self";
  Controller::NodeId expnode1;
  Controller::NodeId expnode2;
  Controller::NodeId expnode3;
  Controller::MembershipEntry expentry1;
  Controller::MembershipEntry expentry2;
  Controller::MembershipEntry expentry3;

  expentry1.count = 1044; // machine
  expentry1.local_time = 1094;
  expentry1.status = Controller::Status::kAlive;
  expentry2.count = 1014;
  expentry2.local_time = 1064;
  expentry2.status = Controller::Status::kSuspected;
  expentry3.count = 1014;
  expentry3.local_time = 1044;
  expentry3.status = Controller::Status::kFailed;
  controller.membership_list_[expnode1] = expentry1;
  controller.membership_list_[expnode2] = expentry2;
  controller.membership_list_[expnode3] = expentry3;
  if(controller.membership_list_.find(expnode1) == controller.membership_list_.end())
       std::cout<<"true";
  else
   std::cout<<"false";
}

int main() {
//   std::cout<<"test0\n";
//   test1();
//   std::cout<<"test1\n";
//      test2();
     std::cout<<"test2\n";
     checkertest1();
     // std::cout<<"test3\n";
     // checkertest2();
     // std::cout<<"test4\n";
     // checkertest3();
     // std::cout<<"test5\n";
}
