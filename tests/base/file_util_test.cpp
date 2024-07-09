#include "lynx/base/file_util.h"

#include <cinttypes>
#include <cstdio>

using namespace lynx;

int main() {
  std::string result;
  int64_t size = 0;
  int err = fs::readFile("/proc/self", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = fs::readFile("/proc/self", 1024, &result, nullptr);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = fs::readFile("/proc/self/cmdline", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = fs::readFile("/dev/null", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = fs::readFile("/dev/zero", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = fs::readFile("/notexist", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = fs::readFile("/dev/zero", 102400, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = fs::readFile("/dev/zero", 102400, &result, nullptr);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
}
