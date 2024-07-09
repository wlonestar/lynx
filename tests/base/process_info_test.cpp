#include "lynx/base/process_info.h"

#include <cinttypes>
#include <cstdio>

int main() {
  printf("pid = %d\n", lynx::process_info::pid());
  printf("uid = %d\n", lynx::process_info::uid());
  printf("euid = %d\n", lynx::process_info::euid());
  printf("start time = %s\n",
         lynx::process_info::startTime().toFormattedString().c_str());
  printf("hostname = %s\n", lynx::process_info::hostname().c_str());
  printf("opened files = %d\n", lynx::process_info::openedFiles());
  printf("threads = %zd\n", lynx::process_info::threads().size());
  printf("num threads = %d\n", lynx::process_info::numThreads());
  printf("status = %s\n", lynx::process_info::procStatus().c_str());
}
