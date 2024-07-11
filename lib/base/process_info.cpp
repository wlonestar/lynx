#include "lynx/base/process_info.h"
#include "lynx/base/current_thread.h"
#include "lynx/base/file_util.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <unistd.h>

namespace lynx {

namespace detail {

thread_local int t_num_opened_files = 0;

int fdDirFilter(const struct dirent *d) {
  if (::isdigit(d->d_name[0]) != 0) {
    ++t_num_opened_files;
  }
  return 0;
}

thread_local std::vector<pid_t> *t_pids = nullptr;

int taskDirFilter(const struct dirent *d) {
  if (::isdigit(d->d_name[0]) != 0) {
    t_pids->push_back(atoi(d->d_name));
  }
  return 0;
}

int scanDir(const char *dirpath, int (*filter)(const struct dirent *)) {
  struct dirent **namelist = nullptr;
  int result = ::scandir(dirpath, &namelist, filter, alphasort);
  assert(namelist == nullptr);
  return result;
}

Timestamp g_start_time = Timestamp::now();
int g_clock_ticks = static_cast<int>(::sysconf(_SC_CLK_TCK));
int g_page_size = static_cast<int>(::sysconf(_SC_PAGE_SIZE));

} // namespace detail

namespace process_info {

pid_t pid() { return ::getpid(); }

std::string pidString() {
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", pid());
  return buf;
}

uid_t uid() { return ::getuid(); }

std::string username() {
  struct passwd pwd;
  struct passwd *result = nullptr;
  char buf[8192];
  const char *name = "unknownuser";

  getpwuid_r(uid(), &pwd, buf, sizeof(buf), &result);
  if (result != nullptr) {
    name = pwd.pw_name;
  }
  return name;
}

uid_t euid() { return ::geteuid(); }

Timestamp startTime() { return detail::g_start_time; }

int clockTicksPerSecond() { return detail::g_clock_ticks; }

int pageSize() { return detail::g_page_size; }

bool isDebugBuild() {
#ifdef NDEBUG
  return false;
#else
  return true;
#endif
}

std::string hostname() {
  char buf[256];
  if (::gethostname(buf, sizeof(buf)) == 0) {
    buf[sizeof(buf) - 1] = '\0';
    return buf;
  }
  return "unknownhost";
}

std::string procname() {
  std::string stat = procStat();
  std::string name;
  size_t lp = stat.find('(');
  size_t rp = stat.rfind(')');
  if (lp != std::string::npos && rp != std::string::npos && lp < rp) {
    name = std::string(stat.data() + lp + 1, static_cast<int>(rp - lp - 1));
  }
  return name;
}

std::string procStatus() {
  std::string result;
  fs::readFile("/proc/self/status", 65536, &result);
  return result;
}

std::string procStat() {
  std::string result;
  fs::readFile("/proc/self/stat", 65536, &result);
  return result;
}

std::string threadStat() {
  char buf[64];
  snprintf(buf, sizeof(buf), "/proc/self/task/%d/stat", current_thread::tid());
  std::string result;
  fs::readFile(buf, 65536, &result);
  return result;
}

std::string exePath() {
  std::string result;
  char buf[1024];
  ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf));
  if (n > 0) {
    result.assign(buf, n);
  }
  return result;
}

int openedFiles() {
  detail::t_num_opened_files = 0;
  detail::scanDir("/proc/self/fd", detail::fdDirFilter);
  return detail::t_num_opened_files;
}

int maxOpenedFiles() {
  struct rlimit rl;
  if (::getrlimit(RLIMIT_NOFILE, &rl) != 0) {
    return openedFiles();
  }
  return static_cast<int>(rl.rlim_cur);
}

CpuTime cpuTime() {
  CpuTime t;
  struct tms tms;
  if (::times(&tms) >= 0) {
    const auto hz = static_cast<double>(clockTicksPerSecond());
    t.user_seconds_ = static_cast<double>(tms.tms_utime) / hz;
    t.system_seconds_ = static_cast<double>(tms.tms_stime) / hz;
  }
  return t;
}

int numThreads() {
  int result = 0;
  std::string status = procStatus();
  size_t pos = status.find("Threads:");
  if (pos != std::string::npos) {
    result = ::atoi(status.c_str() + pos + 8);
  }
  return result;
}

std::vector<pid_t> threads() {
  std::vector<pid_t> result;
  detail::t_pids = &result;
  detail::scanDir("/proc/self/task", detail::taskDirFilter);
  detail::t_pids = nullptr;
  std::sort(result.begin(), result.end());
  return result;
}

} // namespace process_info

} // namespace lynx
