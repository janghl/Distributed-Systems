#include "sender.h"

void test1() {
  Controller controller;
  controller.controller();
  std::map<Controller::NodeId, Controller::MembershipEntry> list1;
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
  list1[node2] = entry2;
  list1[node3] = entry3;
  list2[node4] = entry4;
  list2[node5] = entry5;
  list2[node6] = entry6;
  expected[node7] = entry7;
  expected[node8] = entry8;
  expected[node9] = entry9;
  node2.host = "fa23-cs425-6901.cs.illinois.edu";
  node2.port = "8080";
  node2.time_stamp = 10;
  node3.host = "fa23-cs425-6901.cs.illinois.edu";
  node3.port = "8080";
  node3.time_stamp = 10;
  node4.host = "fa23-cs425-6901.cs.illinois.edu";
  node4.port = "8080";
  node4.time_stamp = 10;
  node5.host = "fa23-cs425-6901.cs.illinois.edu";
  node5.port = "8080";
  node5.time_stamp = 10;
  node6.host = "fa23-cs425-6901.cs.illinois.edu";
  node6.port = "8080";
  node6.time_stamp = 10;
  node7.host = "fa23-cs425-6901.cs.illinois.edu";
  node7.port = "8080";
  node7.time_stamp = 10;
  node8.host = "fa23-cs425-6901.cs.illinois.edu";
  node8.port = "8080";
  node8.time_stamp = 10;
  node9.host = "fa23-cs425-6901.cs.illinois.edu";
  node9.port = "8080";
  node9.time_stamp = 10;
  entry4.count = 1024;
  entry4.local_time = 1025;
  entry4.status = Controller::Status::kAlive;
  entry7.count = 1024;
  entry7.local_time = 1025;
  entry7.status = Controller::Status::kAlive;

  static bool suspicion = false;
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
  int status = controller.Merge();
  // specify fields for expected
  static_assert(list1 == expected);
}
void test2() {
  std::map<Controller::NodeId, Controller::MembershipEntry> list1;
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
  list1[node1] = entry1;
  list1[node2] = entry2;
  list1[node3] = entry3;
  list2[node4] = entry4;
  list2[node5] = entry5;
  list2[node6] = entry6;
  expected[node7] = entry7;
  expected[node8] = entry8;
  expected[node9] = entry9;
  node1.host = "fa23-cs425-6901.cs.illinois.edu";
  node1.port = "8080";
  node1.time_stamp = 10;
  node2.host = "fa23-cs425-6901.cs.illinois.edu";
  node2.port = "8080";
  node2.time_stamp = 10;
  node3.host = "fa23-cs425-6901.cs.illinois.edu";
  node3.port = "8080";
  node3.time_stamp = 10;
  node4.host = "fa23-cs425-6901.cs.illinois.edu";
  node4.port = "8080";
  node4.time_stamp = 10;
  node5.host = "fa23-cs425-6901.cs.illinois.edu";
  node5.port = "8080";
  node5.time_stamp = 10;
  node6.host = "fa23-cs425-6901.cs.illinois.edu";
  node6.port = "8080";
  node6.time_stamp = 10;
  node7.host = "fa23-cs425-6901.cs.illinois.edu";
  node7.port = "8080";
  node7.time_stamp = 10;
  node8.host = "fa23-cs425-6901.cs.illinois.edu";
  node8.port = "8080";
  node8.time_stamp = 10;
  node9.host = "fa23-cs425-6901.cs.illinois.edu";
  node9.port = "8080";
  node9.time_stamp = 10;

  static bool suspicion = true;
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
  int status = merge(list1, list2, machine);
  // specify fields for expected
  static_assert(list1 == expected);
}
void checkertest1() {
  Controller controller; // machine=0, num=1, expect=2
  std::map<Controller::NodeId, Controller::MembershipEntry> list;
  Controller::NodeId node1;
  Controller::NodeId node2;
  Controller::NodeId node3;
  static bool suspicion = false;
  Controller::MembershipEntry entry1;
  Controller::MembershipEntry entry2;
  Controller::MembershipEntry entry3;
  list[node1] = entry1;
  list[node2] = entry2;
  list[node3] = entry3;

  entry1.count = 1044; // machine
  entry1.local_time = 1054;
  entry1.status = Controller::Status::kAlive;
  entry2.count = 1014;
  entry2.local_time = 1004;
  entry2.status = Controller::Status::kAlive;
  // Fix this
  checker(entry1);
  static_assert(list.find(node1) == list.end());

  // make suspicion var public
}
void checkertest3() {
// fix this with public 
  suspicion = true;
  std::map<Controller::NodeId, Controller::MembershipEntry> list;
  Controller::NodeId node1;
  Controller::NodeId node2;
  Controller::NodeId node3;
  Controller::NodeId node4;
  Controller::MembershipEntry entry1;
  Controller::MembershipEntry entry2;
  Controller::MembershipEntry entry3;
  Controller::MembershipEntry entry4;
  list[node1] = entry1;
  list[node2] = entry2;
  list[node3] = entry3;
  list[node4] = entry4;

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
  checker(entry1);
  static_assert(list.find(node1) == list.end());
}
void checkertest2() {
  Controller::MembershipEntry explist;
  Controller::NodeId expnode1;
  Controller::NodeId expnode2;
  Controller::NodeId expnode3;
  Controller::MembershipEntry expentry1;
  Controller::MembershipEntry expentry2;
  Controller::MembershipEntry expentry3;
  explist[expnode1] = expentry1;
  explist[expnode2] = expentry2;
  explist[expnode3] = expentry3;

  expentry1.count = 1044; // machine
  expentry1.local_time = 1094;
  expentry1.status = Controller::Status::kAlive;
  expentry2.count = 1014;
  expentry2.local_time = 1064;
  expentry2.status = Controller::Status::kSuspected;
  expentry3.count = 1014;
  expentry3.local_time = 1044;
  expentry3.status = Controller::Status::kFailed;
  static_assert(list == explist);
}

int main() {
  test1();
  test2();
}
