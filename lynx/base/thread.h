#ifndef LYNX_BASE_THREAD_H
#define LYNX_BASE_THREAD_H

#include "lynx/base/noncopyable.h"

#include <atomic>
#include <functional>
#include <latch>
#include <memory>
#include <thread>

namespace lynx {

class Thread : Noncopyable {
public:
  using ThreadFunc = std::function<void()>;

  explicit Thread(ThreadFunc func, const std::string &name = std::string());
  ~Thread();

  void start();
  void join();

  bool started() const { return started_; }
  pid_t tid() const { return tid_; }
  const std::string &name() const { return name_; }

  static int numCreated() { return num_created; }

private:
  void setDefaultName();

  bool started_;
  bool joined_;
  std::shared_ptr<std::thread> thread_;
  pid_t tid_;
  ThreadFunc func_;
  std::string name_;
  std::latch latch_;

  static std::atomic_int32_t num_created;
};

} // namespace lynx

#endif
