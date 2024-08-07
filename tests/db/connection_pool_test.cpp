#include "lynx/base/thread.h"
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
  auto conn = pool.getConnection();
  auto result = conn->query<Student, uint64_t>().limit(3).toVector();
  for (auto &student : result) {
    LOG_INFO << lynx::serialize(student);
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main() {
  lynx::ConnectionPoolConfig config("127.0.0.1", 5432, "postgres", "123456",
                                    "demo", 2, 4, 10, 5000);

  lynx::EventLoop loop;
  lynx::ConnectionPool pool(&loop, config);
  pool.start();

  for (int i = 0; i < 50; i++) {
    lynx::Thread t1([&] { query(pool); });
    t1.start();
  }

  loop.loop();
}
