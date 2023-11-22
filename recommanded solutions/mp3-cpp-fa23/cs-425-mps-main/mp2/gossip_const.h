#pragma once

// max nodes is 10, so it takes O(log(10)) ~ 4 rounds to spread a gossip message to everyone
// all time outs (fail, suspect, cleanup) need to be O(log(10))
constexpr int FAIL = 8;
constexpr int SUSPECT = 8;
constexpr int CLEANUP = 8;
constexpr int GOSSIP_CYCLE = 500; // in milliseconds
constexpr int GOSSIP_B = 4; // pick random b server in each round
