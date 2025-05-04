#ifndef LYNX_BASE_THREAD_H
#define LYNX_BASE_THREAD_H

#include <atomic>
#include <functional>
#include <latch>
#include <memory>
#include <thread>

namespace lynx {

/**
 * @class Thread
 * @brief Thread class represents a thread of execution.
 *
 * This class encapsulates a thread of execution. It provides a way to create
 * and manage threads in a program.
 */
class Thread {
public:
  using ThreadFunc = std::function<void()>;

  /**
   * @brief Constructs a new Thread object but does not start the thread yet.
   *
   * @param func The function to run in the new thread.
   * @param name An optional name for the thread.
   */
  explicit Thread(ThreadFunc Func, const std::string &Name = std::string());

  /**
   * @brief Destructs the Thread object.
   *
   * If the thread is still running, it detaches the thread.
   */
  ~Thread();

  /**
   * @brief Starts the thread execution.
   *
   * This function initializes a new thread and runs the provided function in
   * it. It sets the thread name using prctl(2) and handles any exceptions that
   * occur during thread creation or execution.
   *
   * @throws std::exception if an exception occurs during thread creation or
   * execution.
   */
  void start();

  /// Waits for the thread to finish.
  void join();

  /// Returns whether the thread has started.
  bool started() const { return Started; }

  pid_t tid() const { return Tid; }
  const std::string &name() const { return Name; }

  /// Returns the total number of Thread objects created.
  static int numCreated() { return NumCreated; }

private:
  /// Sets the default name for the thread.
  void setDefaultName();

  bool Started; /// Indicates whether the thread has started.
  bool Joined;  /// Indicates whether the thread has joined.

  std::shared_ptr<std::thread> Thred; /// The shared pointer to thread object.
  pid_t Tid;                          /// The thread ID.
  ThreadFunc Func;  /// The function to be executed in the thread.
  std::string Name; /// The name of the thread.
  std::latch Latch; /// The latch used for synchronization.

  /// The atomic counter for the number of Thread objects created.
  static std::atomic_int32_t NumCreated;
};

} // namespace lynx

#endif
