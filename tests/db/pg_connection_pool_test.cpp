#include "lynx/base/current_thread.h"
#include "lynx/base/thread_pool.h"
#include "lynx/db/pg_connection_pool.h"
#include "lynx/logger/logging.h"

enum Gender : int {
  Male,
  Female,
};

struct Person {
  short id;      // NOLINT
  char name[10]; // NOLINT
  Gender gender; // NOLINT
  int age;       // NOLINT
  float score;   // NOLINT
} __attribute__((packed));

REFLECTION_TEMPLATE_WITH_NAME(Person, "person", id, name, gender, age, score)

void createTable(lynx::PgConnectionPool &db_pool) {
  auto conn = db_pool.acquire();
  // delete table
  conn->execute("drop table person;");
  // create table
  lynx::KeyMap key_map{"id"};
  lynx::NotNullMap not_null_map;
  not_null_map.fields = {"id", "age"};
  conn->createTable<Person>(key_map, not_null_map);
  // insert data
  Person p1{1, "wjl1", Gender::Female, 30, 101.1F};
  Person p2{2, "wjl2", Gender::Female, 28, 102.2F};
  Person p3{3, "wjl3", Gender::Male, 27, 103.3F};
  Person p4{4, "wjl4", Gender::Female, 26, 104.4F};
  Person p5{5, "wjl1", Gender::Male, 30, 108.1F};
  Person p6{6, "wjl3", Gender::Female, 30, 109.1F};
  conn->insert(p1);
  conn->insert(p2);
  conn->insert(p3);
  conn->insert(p4);
  conn->insert(p5);
  conn->insert(p6);
  db_pool.release(conn);
}

void selectFromDB(lynx::PgConnectionPool &db_pool) {
  auto conn = db_pool.acquire();
  auto pn1 = conn->query<Person>()
                 .where(VALUE(Person::age) > 27 && VALUE(Person::id) < 3)
                 .limit(2)
                 .toVector();
  int usec = rand() % 2 == 0 ? 1000000 : 3000000;
  lynx::current_thread::sleepUsec(usec);
  db_pool.release(conn);
}

void test(int maxSize) {
  LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
  lynx::PgConnectionPool db_pool("127.0.0.1", "5432", "postgres", "123456",
                                 "demo");
  db_pool.start();

  lynx::ThreadPool thread_pool("ThreadPool");
  thread_pool.setMaxQueueSize(maxSize);
  thread_pool.start(15);

  createTable(db_pool);

  for (int i = 0; i < 100; i++) {
    thread_pool.run([&db_pool] { selectFromDB(db_pool); });
  }

  thread_pool.stop();
  db_pool.stop();
}

int main() {
  test(1);
  test(5);
  test(10);
  test(50);
}
