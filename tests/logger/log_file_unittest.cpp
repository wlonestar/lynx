#include "lynx/base/thread.h"
#include "lynx/logger/log_file.h"
#include "lynx/logger/logging.h"

std::unique_ptr<lynx::LogFile> g_log_file;

void outputFunc(const char *msg, int len) { g_log_file->append(msg, len); }

void flushFunc() { g_log_file->flush(); }

std::string line =
    "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

void threadFunc() {
  for (int i = 0; i < 1000; i++) {
    LOG_INFO << line << i;
    usleep(1000);
  }
}

int main(int argc, char *argv[]) {
  char name[256] = {'\0'};
  strncpy(name, argv[0], sizeof(name) - 1);
  g_log_file = std::make_unique<lynx::LogFile>(::basename(name), 200 * 1000);

  lynx::Logger::setOutput(outputFunc);
  lynx::Logger::setFlush(flushFunc);

  lynx::Thread t1(threadFunc);
  lynx::Thread t2(threadFunc);
  lynx::Thread t3(threadFunc);
  lynx::Thread t4(threadFunc);
  lynx::Thread t5(threadFunc);

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
}
