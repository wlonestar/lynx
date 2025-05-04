#include "lynx/base/ThreadPool.h"
#include "lynx/base/Thread.h"

#include <cassert>

#include <fmt/format.h>
#include <string>

namespace lynx {

ThreadPool::ThreadPool(const std::string &Name)
    : Name(Name), MaxQueueSize(0), Running(false) {}

ThreadPool::~ThreadPool() {
  if (Running)
    stop();
}

void ThreadPool::start(int NumThreads) {
  assert(Threads.empty());
  Running = true;

  Threads.reserve(NumThreads);
  for (int I = 0; I < NumThreads; ++I) {
    std::string Id = std::to_string(I + 1);
    /// Create a new thread and add it to the vector
    Threads.emplace_back(new Thread([&] { runInThread(); }, Name + Id));
    Threads[I]->start();
  }

  /// If there are no threads and a thread_init_callback function is provided,
  /// call the callback function immediately
  if (NumThreads == 0 && ThreadInitCallback)
    ThreadInitCallback();
}

void ThreadPool::stop() {
  {
    std::lock_guard<std::mutex> Lock(Mutex);
    Running = false;
    NotEmpty.notify_all();
    NotFull.notify_all();
  }

  for (auto &Thread : Threads)
    Thread->join();
}

size_t ThreadPool::queueSize() const {
  std::lock_guard<std::mutex> Lock(Mutex);
  return Queue.size();
}

void ThreadPool::run(Task Task) {
  if (Threads.empty()) {
    Task();
  } else {
    std::unique_lock<std::mutex> Lock(Mutex);
    /// If the queue is full and the thread pool is running, wait until
    /// there is space available in the queue.
    while (isFull() && Running)
      NotFull.wait(Lock);

    /// If the thread pool is not running, return immediately.
    if (!Running)
      return;

    /// At this point, we know that there is space available in the queue,
    /// so add the task to the queue and notify the first available thread.
    assert(!isFull());
    Queue.push_back(std::move(Task));
    NotEmpty.notify_one();
  }
}

bool ThreadPool::isFull() const {
  return MaxQueueSize > 0 && Queue.size() >= MaxQueueSize;
}

void ThreadPool::runInThread() {
  try {
    /// If a thread initialization callback is provided, call it.
    if (ThreadInitCallback)
      ThreadInitCallback();

    /// Continuously take tasks from the task queue and execute them.
    while (Running) {
      Task Task(take()); /// Take a task from the task queue.
      /// If a task is available, execute it.
      if (Task)
        Task();
    }
  } catch (const std::exception &Ex) {
    fprintf(stderr, "exception caught in ThreadPool %s\n", Name.c_str());
    fprintf(stderr, "reason: %s\n", Ex.what());
    abort();
  } catch (...) {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n",
            Name.c_str());
    throw;
  }
}

ThreadPool::Task ThreadPool::take() {
  std::unique_lock<std::mutex> Lock(Mutex);
  /// Wait until a task is available or the thread pool has stopped running.
  while (Queue.empty() && Running)
    NotEmpty.wait(Lock);

  Task Task; /// The task to be returned.
  /// If there is at least one task in the queue, retrieve and remove it.
  if (!Queue.empty()) {
    Task = Queue.front();
    Queue.pop_front();
    /// If the maximum queue size is set, notify the not_full condition
    /// variable.
    if (MaxQueueSize > 0)
      NotFull.notify_one();
  }

  return Task;
}

} // namespace lynx
