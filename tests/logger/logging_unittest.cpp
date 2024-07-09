#include "lynx/base/thread.h"
#include "lynx/base/timestamp.h"
#include "lynx/logger/log_file.h"
#include "lynx/logger/logging.h"

#include <cstdio>
#include <memory>
#include <unistd.h>

int g_total;
FILE *g_file;
std::unique_ptr<lynx::LogFile> g_log_file;

void dummyOutput(const char *msg, int len) {
  g_total += len;
  if (g_file != nullptr) {
    ::fwrite(msg, 1, len, g_file);
  } else if (g_log_file) {
    g_log_file->append(msg, len);
  }
}

void bench(const char *type) {
  lynx::Logger::setOutput(dummyOutput);
  lynx::Timestamp start(lynx::Timestamp::now());
  g_total = 0;

  int n = 1000 * 1000;
  const bool long_log = false;
  std::string empty = " ";
  std::string long_str(3000, 'X');
  long_str += " ";

  for (int i = 0; i < n; i++) {
    LOG_INFO << "Hello 0123456789"
             << " abcdefghijklmnopqrstuvwxyz" << (long_log ? long_str : empty)
             << (114.514 + i) << i;
  }
  lynx::Timestamp end(lynx::Timestamp::now());
  double seconds = lynx::timeDifference(end, start);
  printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n", type,
         seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void logInThread() {
  LOG_INFO << "logInThread";
  usleep(1000);
}

int main() {
  getppid();

  lynx::Thread t1(logInThread);
  lynx::Thread t2(logInThread);
  lynx::Thread t3(logInThread);
  lynx::Thread t4(logInThread);
  lynx::Thread t5(logInThread);

  t1.start();
  t2.start();
  t3.start();
  t4.start();
  t5.start();

  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();

  LOG_TRACE << "trace";
  LOG_DEBUG << "debug";
  LOG_INFO << "Hello";
  LOG_WARN << "World";
  LOG_ERROR << "Error";
  LOG_INFO << sizeof(lynx::Logger);
  LOG_INFO << sizeof(lynx::LogStream);
  LOG_INFO << sizeof(lynx::LogStream::Buffer);

  sleep(1);
  bench("nop");

  char buffer[64 * 1024];

  g_file = ::fopen("/dev/null", "w");
  setbuffer(g_file, buffer, sizeof(buffer));
  bench("/dev/null");
  ::fclose(g_file);

  g_file = ::fopen("/tmp/log", "w");
  setbuffer(g_file, buffer, sizeof(buffer));
  bench("/tmp/log");
  ::fclose(g_file);

  g_file = nullptr;
  g_log_file =
      std::make_unique<lynx::LogFile>("test_log_st", 500 * 1000 * 1000, false);
  bench("test_log_st");

  g_log_file =
      std::make_unique<lynx::LogFile>("test_log_mt", 500 * 1000 * 1000, true);
  bench("test_log_mt");
  g_log_file.reset();
}
