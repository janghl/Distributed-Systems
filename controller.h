#pragma once

#include <chrono>
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <numeric>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "client_utils.h"

class Controller {
public:
  Controller(char *pattern, size_t num_machines)
      : pattern_(pattern), num_machines_(num_machines) {}

  /**
   * Queries servers on VMs for grep output and prints on this VM.
   */
  void DistributedGrep() {
    // 0 on success 1 on failure
    std::vector<std::future<int>> futures;
    futures.reserve(num_machines_);
    const auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < num_machines_; ++i) {
      // Start async background threads to query each server
      futures.push_back(std::async(client_utils::QueryServer, pattern_, i));
    }

    std::vector<size_t> counter(10);
    for (size_t i = 0; i < num_machines_; ++i) {
      // Background threads wait here
      if (futures[i].get())
        continue;

      std::ifstream file;
      const std::string file_name = std::to_string(i + 1) + ".temp";
      file.open(file_name);

      std::string line;
      while (std::getline(file, line)) {
        ++counter[i];
        std::cout << line;
      }
    }
    const auto end_time = std::chrono::high_resolution_clock::now();

    PrintStats(start_time, end_time, counter);
  }

private:
  void
  PrintStats(const std::chrono::time_point<std::chrono::high_resolution_clock>
                 &start_time,
             const std::chrono::time_point<std::chrono::high_resolution_clock>
                 &end_time,
             std::vector<size_t> &counter) const {
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    std::cout << "STAT" << std::endl;
    for (size_t i = 0; i < num_machines_; ++i) {
      std::printf("VM %lu matched %lu lines\n", i + 1, counter[i]);
    }

    std::printf("Matched %d lines total in %ld ms",
                std::accumulate(counter.begin(), counter.end(), 0),
                duration.count());
  }

  const char *pattern_;
  const size_t num_machines_;
};