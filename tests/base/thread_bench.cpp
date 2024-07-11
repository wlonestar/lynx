#include "lynx/base/blocking_queue.h"
#include "lynx/base/current_thread.h"
#include "lynx/base/thread.h"
#include "lynx/base/timestamp.h"

#include <atomic>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

bool g_verbose = false;
std::mutex g_mutex;
std::atomic_int32_t g_count;
std::map<int, int> g_delays;

void threadFunc() {
  // printf("tid=%d\n", lynx::CurrentThread::tid());
  g_count.fetch_add(1);
}

void threadFunc2(lynx::Timestamp start) {
  lynx::Timestamp cur(lynx::Timestamp::now());
  int delay = static_cast<int>(timeDifference(cur, start) * 1000000);
  std::lock_guard<std::mutex> lock(g_mutex);
  ++g_delays[delay];
}

void forkBench() {
  sleep(10);
  lynx::Timestamp start(lynx::Timestamp::now());
  int k_processes = 10 * 1000;

  printf("Creating %d processes in serial\n", k_processes);
  for (int i = 0; i < k_processes; ++i) {
    pid_t child = fork();
    if (child == 0) {
      exit(0);
    } else {
      waitpid(child, nullptr, 0);
    }
  }

  double time_used = timeDifference(lynx::Timestamp::now(), start);
  printf("time elapsed %.3f seconds, process creation time used %.3f us\n",
         time_used, time_used * 1e6 / k_processes);
  printf("number of created processes %d\n", k_processes);
}

class Bench {
public:
  Bench(int numThreads) : start_latch_(numThreads), stop_latch_(1) {
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
      char name[32];
      snprintf(name, sizeof(name), "work thread %d", i);
      threads_.emplace_back(
          new lynx::Thread([this] { threadFunc(); }, std::string(name)));
    }
  }

  void start() {
    const int num_threads = static_cast<int>(threads_.size());
    printf("Creating %d threads in parallel\n", num_threads);
    lynx::Timestamp start = lynx::Timestamp::now();

    for (auto &thr : threads_) {
      thr->start();
    }
    start_latch_.wait();
    double time_used = timeDifference(lynx::Timestamp::now(), start);
    printf("all %d threads started, %.3fms total, %.3fus per thread\n",
           num_threads, 1e3 * time_used, 1e6 * time_used / num_threads);

    TimestampQueue::queue_type queue = start_.drain();
    if (g_verbose) {
      // for (const auto& [tid, ts] : queue)
      for (const auto &e : queue) {
        printf("thread %d, %.0f us\n", e.first,
               timeDifference(e.second, start) * 1e6);
      }
    }
  }

  void stop() {
    lynx::Timestamp stop = lynx::Timestamp::now();
    stop_latch_.count_down();
    for (auto &thr : threads_) {
      thr->join();
    }

    lynx::Timestamp t2 = lynx::Timestamp::now();
    printf("all %zd threads joined, %.3fms\n", threads_.size(),
           1e3 * timeDifference(t2, stop));
    TimestampQueue::queue_type queue = done_.drain();
    if (g_verbose) {
      // for (const auto& [tid, ts] : queue)
      for (const auto &e : queue) {
        printf("thread %d, %.0f us\n", e.first,
               timeDifference(e.second, stop) * 1e6);
      }
    }
  }

private:
  void threadFunc() {
    const int tid = lynx::current_thread::tid();
    start_.put(std::make_pair(tid, lynx::Timestamp::now()));
    start_latch_.count_down();
    stop_latch_.wait();
    done_.put(std::make_pair(tid, lynx::Timestamp::now()));
  }

  using TimestampQueue = lynx::BlockingQueue<std::pair<int, lynx::Timestamp>>;
  TimestampQueue start_, run_, done_;
  std::latch start_latch_, stop_latch_;
  std::vector<std::unique_ptr<lynx::Thread>> threads_;
};

int main(int argc, char *argv[]) {
  forkBench();

  g_verbose = argc > 1;
  printf("pid=%d, tid=%d, verbose=%d\n", ::getpid(),
         lynx::current_thread::tid(), static_cast<int>(g_verbose));
  lynx::Timestamp start(lynx::Timestamp::now());

  int k_threads = 100 * 1000;
  printf("Creating %d threads in serial\n", k_threads);
  for (int i = 0; i < k_threads; ++i) {
    lynx::Thread t1(threadFunc);
    t1.start();
    t1.join();
  }

  double time_used = timeDifference(lynx::Timestamp::now(), start);
  printf("elapsed %.3f seconds, thread creation time %.3f us\n", time_used,
         time_used * 1e6 / k_threads);
  int count = g_count;
  printf("number of created threads %d, g_count = %d\n",
         lynx::Thread::numCreated(), count);

  for (int i = 0; i < k_threads; ++i) {
    lynx::Timestamp now(lynx::Timestamp::now());
    lynx::Thread t2([now] { return threadFunc2(now); });
    t2.start();
    t2.join();
  }

  if (g_verbose) {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (const auto &delay : g_delays) {
      printf("delay = %d, count = %d\n", delay.first, delay.second);
    }
  }

  Bench t(10000);
  t.start();
  t.stop();
}
