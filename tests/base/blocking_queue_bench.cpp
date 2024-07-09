#include "lynx/base/blocking_queue.h"
#include "lynx/base/current_thread.h"
#include "lynx/base/thread.h"
#include "lynx/base/timestamp.h"
#include "lynx/logger/logging.h"

#include <cstdio>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>

bool g_verbose = false;

// Many threads, one queue.
class Bench {
public:
  Bench(int numThreads) : latch_(numThreads) {
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.emplace_back(
          new lynx::Thread([this] { threadFunc(); }, std::string(name)));
    }
    for (auto &thr : threads_) {
      thr->start();
    }
  }

  void run(int times) {
    printf("waiting for count down latch\n");
    latch_.wait();
    LOG_INFO << threads_.size() << " threads started";
    int64_t total_delay = 0;
    for (int i = 0; i < times; ++i) {
      lynx::Timestamp now(lynx::Timestamp::now());
      queue_.put(now);
      total_delay += delay_queue_.take();
    }
    printf("Average delay: %.3fus\n", static_cast<double>(total_delay) / times);
  }

  void joinAll() {
    for (size_t i = 0; i < threads_.size(); ++i) {
      queue_.put(lynx::Timestamp::invalid());
    }

    for (auto &thr : threads_) {
      thr->join();
    }
    LOG_INFO << threads_.size() << " threads stopped";
  }

private:
  void threadFunc() {
    if (g_verbose) {
      printf("tid=%d, %s started\n", lynx::current_thread::tid(),
             lynx::current_thread::name());
    }

    std::map<int, int> delays;
    latch_.count_down();
    bool running = true;
    while (running) {
      lynx::Timestamp t(queue_.take());
      lynx::Timestamp now(lynx::Timestamp::now());
      if (t.valid()) {
        int delay = static_cast<int>(timeDifference(now, t) * 1000000);
        // printf("tid=%d, latency = %d us\n",
        //        lynx::CurrentThread::tid(), delay);
        ++delays[delay];
        delay_queue_.put(delay);
      }
      running = t.valid();
    }

    if (g_verbose) {
      printf("tid=%d, %s stopped\n", lynx::current_thread::tid(),
             lynx::current_thread::name());
      for (const auto &delay : delays) {
        printf("tid = %d, delay = %d, count = %d\n",
               lynx::current_thread::tid(), delay.first, delay.second);
      }
    }
  }

  lynx::BlockingQueue<lynx::Timestamp> queue_;
  lynx::BlockingQueue<int> delay_queue_;
  std::latch latch_;
  std::vector<std::unique_ptr<lynx::Thread>> threads_;
};

int main(int argc, char *argv[]) {
  int threads = argc > 1 ? atoi(argv[1]) : 1;

  Bench t(threads);
  t.run(100000);
  t.joinAll();
}
