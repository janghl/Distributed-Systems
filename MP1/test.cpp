#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include <fcntl.h>

#include "controller.h"

int CountPattern(char *pattern, FILE *f) {
  rewind(f);
  f = fopen("test.txt", "w+");
  Controller controller(pattern, 4);
  controller.DistributedGrep();
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int count = 0;
  while ((read = getline(&line, &len, f)) != -1) {
    if (std::string{line}.find(pattern) != std::string::npos) {
      ++count;
    }
  }
  return count;
}

int main(int argc, char *argv[]) {
  char file_name[] = "test.txt";
  FILE *f = freopen(file_name, "w+", stdout);
  char rare_pattern[] = "rare-pattern";
  char somewhat_frequent_pattern[] = "somewhat-frequent-pattern";
  char frequent_pattern[] = "frequent-pattern";
  int rare_count = CountPattern(rare_pattern, f);
  int somewhat_frequent_count = CountPattern(somewhat_frequent_pattern, f);
  int frequent_count = CountPattern(frequent_pattern, f);
  assert(rare_count < somewhat_frequent_count);
  assert(somewhat_frequent_count < frequent_count);
  char one_log_pattern[] = "one-log";
  char some_logs_pattern[] = "some-logs";
  char all_logs_pattern[] = "all-logs";
  int one_log_count = CountPattern(one_log_pattern, f);
  int some_logs_count = CountPattern(some_logs_pattern, f);
  int all_logs_count = CountPattern(all_logs_pattern, f);
  assert(900 < one_log_count && one_log_count < 1100);
  assert(1900 < some_logs_count && some_logs_count < 2100);
  assert(3900 < all_logs_count && all_logs_count < 4100);
  return 0;
}