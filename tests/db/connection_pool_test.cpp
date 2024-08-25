#include "lynx/base/thread.h"
#include "lynx/base/thread_pool.h"
#include "lynx/db/connection.h"
#include "lynx/db/connection_pool.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/orm/reflection.h"

#include <chrono>
#include <thread>

enum Gender : int {
  Male,
  Female,
};

struct Student {
  uint64_t id;       // NOLINT
  std::string name;  // NOLINT
  Gender gender;     // NOLINT
  int entry_year;    // NOLINT
  std::string major; // NOLINT
  double gpa;        // NOLINT
} __attribute__((packed));

REFLECTION_TEMPLATE_WITH_NAME(Student, "student", id, name, gender, entry_year,
                              major, gpa)
REGISTER_AUTO_KEY(Student, id)

void query(lynx::ConnectionPool &pool) {
  auto conn = pool.acquire();
  auto result =
      conn->query<Student, uint64_t>().limit(rand() % 10 + 1).toVector();
  LOG_INFO << "query " << result.size() << " records";
}

int main() {
  lynx::ConnectionPoolConfig config("127.0.0.1", 5432, "postgres", "123456",
                                    "demo", 2, 4, 10, 5000);

  lynx::ConnectionPool connection_pool(config);
  connection_pool.start();

  lynx::ThreadPool thread_pool("ThreadPool");
  thread_pool.setMaxQueueSize(5);
  thread_pool.start(3);

  LOG_WARN << "Querying";
  for (int i = 0; i < 100; i++) {
    thread_pool.run([&] { query(connection_pool); });
  }
  LOG_WARN << "Done";

  std::latch latch(1);
  thread_pool.run([&] { latch.count_down(); });
  latch.wait();

  thread_pool.stop();
  connection_pool.stop();
}
