#include "student.h"

#include "lynx/db/pg_connection_pool.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/logger/async_logging.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/web/web_server.h"

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <utility>

off_t roll_size = 500 * 1000 * 1000;
lynx::AsyncLogging *g_async_log = nullptr;

void asyncOutput(const char *msg, int len) { g_async_log->append(msg, len); }

void initDb(lynx::PgConnectionPool &pool);

int main(int argc, char *argv[]) {
  /// Init async logging
  char name[256] = {'\0'};
  strncpy(name, argv[0], sizeof(name) - 1);
  lynx::AsyncLogging log(::basename(name), roll_size);
  lynx::Logger::setOutput(asyncOutput);
  log.start();
  g_async_log = &log;

  /// Init Web Server
  int num_threads = 5;
  if (argc > 1) {
    num_threads = atoi(argv[1]);
  }
  lynx::PgConnectionPool pool("127.0.0.1", "5432", "postgres", "123456",
                              "demo");
  lynx::EventLoop loop;
  lynx::WebServer server(&loop, lynx::InetAddress(8000), "WebServer", &pool);
  server.setThreadNum(num_threads);
  LOG_INFO << "start HTTP server with " << num_threads << " threads";

  /// Setup server and pool
  server.start();
  initDb(pool);

  /// Register handlers
  StudentRepository repository(pool);
  StudentService service(repository);
  StudentController student_controller(service);
  student_controller.registr(server, student_controller);

  loop.loop();
}

void initDb(lynx::PgConnectionPool &pool) {
  /// Connect database
  lynx::PgConnection conn("PgConnection");
  conn.connect("localhost", "5432", "postgres", "123456", "demo");

  /// Create table (drop if table already exists)
  conn.execute("drop table student; drop sequence student_id_seq;");
  lynx::AutoKeyMap key_map{"id"};
  lynx::NotNullMap not_null_map;
  not_null_map.fields = {"id", "name", "gender", "entry_year"};
  bool flag = conn.createTable<Student>(key_map, not_null_map);
  if (!flag) {
    abort();
  }

  /// Insert 1: insert one
  Student s1{2024033001, "Li Ming 1", Gender::Male, 2024, "SE", 3.8};
  Student s2{2024033002, "Li Ming 2", Gender::Male, 2024, "SE", 3.82};
  Student s3{2024033003, "Li Ming 3", Gender::Female, 2024, "CS", 3.78};
  Student s4{2023033024, "Liu Hua 1", Gender::Male, 2023, "SE", 3.98};
  Student s5{2024033035, "Liu Hua 2", Gender::Female, 2023, "CS", 3.89};
  conn.insert(s1);
  conn.insert(s2);
  conn.insert(s3);
  conn.insert(s4);
  conn.insert(s5);

  /// Insert 2: insert many
  std::vector<Student> students;
  for (int i = 0; i < 10; i++) {
    Student s;
    s.id = 2023033001 + i;
    s.name = "Che hen " + std::to_string(i);
    s.gender = rand() % 2 == 0 ? Gender::Female : Gender::Male;
    s.entry_year = 2023;
    s.major = rand() % 2 == 0 ? "CS" : "SE";
    s.gpa = 3.5 + (rand() % 10) * 0.05;
    students.push_back(s);
  }
  conn.insert(students);
}
