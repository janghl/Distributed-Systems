const int check_merge() {
    Controller controller;
    membership_list_ list1;
    // specify fields for list1
    membership_list_ list2;
    // specify fields for list2
    membership_list_ expected;
    struct NodeId node1;
    struct NodeId node2;
    struct NodeId node3;
    struct NodeId node4;
    struct NodeId node5;
    struct NodeId node6;
    struct NodeId node7;
    struct NodeId node8;
    struct NodeId node9;
    struct MembershipEntry entry1;
    struct MembershipEntry entry2;
    struct MembershipEntry entry3;
    struct MembershipEntry entry4;
    struct MembershipEntry entry5;
    struct MembershipEntry entry6;
    struct MembershipEntry entry7;
    struct MembershipEntry entry8;
    struct MembershipEntry entry9;
    list1[node1]=entry1;
    list1[node2]=entry2;
    list1[node3]=entry3;
    list2[node4]=entry4;
    list2[node5]=entry5;
    list2[node6]=entry6;
    expected[node7]=entry7;
    expected[node8]=entry8;
    expected[node9]=entry9;
    node1.ip=NULL;
    node1.port=NULL;
    node1.time_stamp=NULL;
    node2.ip="fa23-cs425-6901.cs.illinois.edu";
    node2.port="8080";
    node2.time_stamp="10";
    node3.ip="fa23-cs425-6901.cs.illinois.edu";
    node3.port="8080";
    node3.time_stamp="10";
    node4.ip="fa23-cs425-6901.cs.illinois.edu";
    node4.port="8080";
    node4.time_stamp="10";
    node5.ip="fa23-cs425-6901.cs.illinois.edu";
    node5.port="8080";
    node5.time_stamp="10";
    node6.ip="fa23-cs425-6901.cs.illinois.edu";
    node6.port="8080";
    node6.time_stamp="10";
    node7.ip="fa23-cs425-6901.cs.illinois.edu";
    node7.port="8080";
    node7.time_stamp="10";
    node8.ip="fa23-cs425-6901.cs.illinois.edu";
    node8.port="8080";
    node8.time_stamp="10";
    node9.ip="fa23-cs425-6901.cs.illinois.edu";
    node9.port="8080";
    node9.time_stamp="10";
    entry4.count=1024;
    entry4.local_time=1025;
    entry4.status=kAlive;
    entry7.count=1024;
    entry7.local_time=1025;
    entry7.status=kAlive;
    entry1.count=1024;
    entry1.local_time=1025;
    entry1.status=kAlive;

    static bool suspicion = false;
    entry5.status=kFailed;
    entry8.status=kFailed;
    entry2.status=kAlive;
    entry5.count=1024;
    entry5.local_time=1025;
    entry2.count=995;
    entry2.local_time=246;
    entry8.count=995;
    entry8.local_time=246;
    entry6.status=kAlive;
    entry9.status=kAlive;
    entry3.status=kAlive;
    entry6.count=1024;
    entry6.local_time=1025;
    entry3.count=995;
    entry3.local_time=246;
    entry9.count=1024;
    entry9.local_time=1025;
    int status = merge(list1, list2, machine);
    // specify fields for expected
    static_assert(list1 == expected);
    

    membership_list_ list1;
    // specify fields for list1
    membership_list_ list2;
    // specify fields for list2
    membership_list_ expected;
    struct NodeId node1;
    struct NodeId node2;
    struct NodeId node3;
    struct NodeId node4;
    struct NodeId node5;
    struct NodeId node6;
    struct NodeId node7;
    struct NodeId node8;
    struct NodeId node9;
    struct MembershipEntry entry1;
    struct MembershipEntry entry2;
    struct MembershipEntry entry3;
    struct MembershipEntry entry4;
    struct MembershipEntry entry5;
    struct MembershipEntry entry6;
    struct MembershipEntry entry7;
    struct MembershipEntry entry8;
    struct MembershipEntry entry9;
    list1[node1]=entry1;
    list1[node2]=entry2;
    list1[node3]=entry3;
    list2[node4]=entry4;
    list2[node5]=entry5;
    list2[node6]=entry6;
    expected[node7]=entry7;
    expected[node8]=entry8;
    expected[node9]=entry9;
    node1.ip="fa23-cs425-6901.cs.illinois.edu";
    node1.port="8080";
    node1.time_stamp="10";
    node2.ip="fa23-cs425-6901.cs.illinois.edu";
    node2.port="8080";
    node2.time_stamp="10";
    node3.ip="fa23-cs425-6901.cs.illinois.edu";
    node3.port="8080";
    node3.time_stamp="10";
    node4.ip="fa23-cs425-6901.cs.illinois.edu";
    node4.port="8080";
    node4.time_stamp="10";
    node5.ip="fa23-cs425-6901.cs.illinois.edu";
    node5.port="8080";
    node5.time_stamp="10";
    node6.ip="fa23-cs425-6901.cs.illinois.edu";
    node6.port="8080";
    node6.time_stamp="10";
    node7.ip="fa23-cs425-6901.cs.illinois.edu";
    node7.port="8080";
    node7.time_stamp="10";
    node8.ip="fa23-cs425-6901.cs.illinois.edu";
    node8.port="8080";
    node8.time_stamp="10";
    node9.ip="fa23-cs425-6901.cs.illinois.edu";
    node9.port="8080";
    node9.time_stamp="10";

    static bool suspicion = true;
    entry4.status=kFailed;
    entry7.status=kFailed;
    entry1.status=kFailed;
    entry4.count=1024;
    entry4.local_time=1025;
    entry1.count=995;
    entry1.local_time=246;
    entry7.count=995;
    entry7.local_time=246;
    
    entry5.status=kFailed;
    entry8.status=kFailed;
    entry2.status=kAlive;
    entry5.count=1024;
    entry5.local_time=1025;
    entry2.count=995;
    entry2.local_time=246;
    entry8.count=995;
    entry8.local_time=246;
    
    entry6.status=kSuspected;
    entry9.status=kSuspected;
    entry3.status=kAlive;
    entry6.count=1024;
    entry6.local_time=1025;
    entry3.count=995;
    entry3.local_time=246;
    entry9.count=1024;
    entry9.local_time=1025;
    int status = merge(list1, list2, machine);
    // specify fields for expected
    static_assert(list1 == expected);
}









const int check_checker() {
    Controller controller;          //machine=0, num=1, expect=2
    MembershipEntry list;
    struct NodeId node1;
    struct NodeId node2;
    struct NodeId node3;
    static bool suspicion = false;
    struct MembershipEntry entry1;
    struct MembershipEntry entry2;
    struct MembershipEntry entry3;
    list[node1]=entry1;
    list[node2]=entry2;
    list[node3]=entry3;

    entry1.count=1044;         //machine
    entry1.local_time=1054;
    entry1.status=kAlive;
    entry2.count=1014;
    entry2.local_time=1004;
    entry2.status=kAlive;
    checker(entry1);
    static_assert(list.find(node1)==list.end());

    
    static bool suspicion = true;
    MembershipEntry list;
    struct NodeId node1;
    struct NodeId node2;
    struct NodeId node3;
    struct NodeId node4;
    struct MembershipEntry entry1;
    struct MembershipEntry entry2;
    struct MembershipEntry entry3;
    struct MembershipEntry entry4;
    list[node1]=entry1;
    list[node2]=entry2;
    list[node3]=entry3;
    list[node4]=entry4;


    entry1.count=1044;         //machine
    entry1.local_time=1094;
    entry1.status=kAlive;
    entry2.count=1014;
    entry2.local_time=1064;
    entry2.status=kAlive;
    entry3.count=1014;
    entry3.local_time=1044;
    entry3.status=kAlive;
    entry4.count=1014;
    entry4.local_time=1024;
    entry4.status=kAlive;
    checker(entry1);
    static_assert(list.find(node1)==list.end());


    MembershipEntry explist;
    struct NodeId expnode1;
    struct NodeId expnode2;
    struct NodeId expnode3;
    struct MembershipEntry expentry1;
    struct MembershipEntry expentry2;
    struct MembershipEntry expentry3;
    explist[expnode1]=expentry1;
    explist[expnode2]=expentry2;
    explist[expnode3]=expentry3;


    expentry1.count=1044;         //machine
    expentry1.local_time=1094;
    expentry1.status=kAlive;
    expentry2.count=1014;
    expentry2.local_time=1064;
    expentry2.status=kSuspected;
    expentry3.count=1014;
    expentry3.local_time=1044;
    expentry3.status=kFalied;
    static_assert(list==explist);

    
}