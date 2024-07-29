#include "lynx/base/timestamp.h"
#include "lynx/logger/async_logging.h"
#include "lynx/logger/logging.h"

off_t roll_size = 500 * 1000 * 1000;
lynx::AsyncLogging *g_async_log = nullptr;

void asyncOutput(const char *msg, int len) { g_async_log->append(msg, len); }

void bench(bool longLog) {
  lynx::Logger::setOutput(asyncOutput);

  int cnt = 0;
  const int batch = 1000;
  std::string empty = " ";
  std::string long_str(3000, 'X');
  long_str += " ";

  for (int t = 0; t < 30; t++) {
    lynx::Timestamp start = lynx::Timestamp::now();
    for (int i = 0; i < batch; i++) {
      LOG_INFO << "Hello 0123456789"
               << " abcdefghijklmnopqrstuvwxyz " << (longLog ? long_str : empty)
               << cnt;
      cnt++;
    }
    lynx::Timestamp end = lynx::Timestamp::now();
    printf("%f\n", lynx::timeDiff(end, start) * 1000000 / batch);
    struct timespec ts = {0, 500 * 1000 * 1000};
    nanosleep(&ts, nullptr);
  }
}

int main(int argc, char *argv[]) {
  printf("pid = %d\n", getpid());

  char name[256] = {'\0'};
  strncpy(name, argv[0], sizeof(name) - 1);
  lynx::AsyncLogging log(::basename(name), roll_size);
  log.start();
  g_async_log = &log;

  bool long_log = argc > 1;
  bench(long_log);
}
