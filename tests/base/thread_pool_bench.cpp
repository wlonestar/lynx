#include "lynx/base/current_thread.h"
#include "lynx/base/thread_pool.h"
#include "lynx/logger/logging.h"

void print() { printf("tid=%d\n", lynx::current_thread::tid()); }

void printString(const std::string &str) {
  LOG_INFO << str;
  usleep(100 * 1000);
}

void test(int maxSize) {
  LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
  lynx::ThreadPool pool("MainThreadPool");
  pool.setMaxQueueSize(maxSize);
  pool.start(5);

  LOG_WARN << "Adding";
  pool.run(print);
  pool.run(print);
  for (int i = 0; i < 100; ++i) {
    char buf[32];
    snprintf(buf, sizeof(buf), "task %d", i);
    pool.run([capture0 = std::string(buf)] { return printString(capture0); });
  }
  LOG_WARN << "Done";

  std::latch latch(1);
  pool.run([&latch] { latch.count_down(); });
  latch.wait();
  pool.stop();
}

/*
 * Wish we could do this in the future.
void testMove()
{
  lynx::ThreadPool pool;
  pool.start(2);

  std::unique_ptr<int> x(new int(42));
  pool.run([y = std::move(x)]{ printf("%d: %d\n", lynx::CurrentThread::tid(),
*y); }); pool.stop();
}
*/

void longTask(int num) {
  LOG_INFO << "longTask " << num;
  std::this_thread::sleep_for(std::chrono::microseconds(3000000));
}

void test2() {
  LOG_WARN << "Test ThreadPool by stoping early.";
  lynx::ThreadPool pool("ThreadPool");
  pool.setMaxQueueSize(5);
  pool.start(3);

  lynx::Thread thread1(
      [&pool]() {
        for (int i = 0; i < 20; ++i) {
          pool.run([i] { return longTask(i); });
        }
      },
      "thread1");
  thread1.start();

  std::this_thread::sleep_for(std::chrono::microseconds(5000000));
  LOG_WARN << "stop pool";
  pool.stop(); // early stop

  thread1.join();
  // run() after stop()
  pool.run(print);
  LOG_WARN << "test2 Done";
}

int main() {
  test(0);
  test(1);
  test(5);
  test(10);
  test(50);
  test2();
}
