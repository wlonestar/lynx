#ifndef LYNX_BASE_PROCESS_INFO_H
#define LYNX_BASE_PROCESS_INFO_H

#include "lynx/base/timestamp.h"

#include <string>
#include <vector>

namespace lynx::process_info {

pid_t pid();
std::string pidString();
uid_t uid();
std::string username();
uid_t euid();
Timestamp startTime();
int clockTicksPerSecond();
int pageSize();
bool isDebugBuild();

std::string hostname();
std::string procname();

std::string procStatus();
std::string procStat();
std::string threadStat();
std::string exePath();

int openedFiles();
int maxOpenedFiles();

struct CpuTime {
  double user_seconds_{};
  double system_seconds_{};

  CpuTime() = default;

  double total() const { return user_seconds_ + system_seconds_; }
};

CpuTime cpuTime();
int numThreads();
std::vector<pid_t> threads();

} // namespace lynx::process_info

#endif
