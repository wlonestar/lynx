#include "student.h"

#include "lynx/web/web_server.h"

void initDb(lynx::ConnectionPool &pool) {
  auto conn = pool.getConnection();

  /// Create table (drop if table already exists)
  conn->execute("drop table student; drop sequence student_id_seq;");
  lynx::AutoKeyMap key_map{"id"};
  lynx::NotNullMap not_null_map;
  not_null_map.fields = {"id", "name", "gender", "entry_year"};
  bool flag = conn->createTable<Student>(key_map, not_null_map);
  if (!flag) {
    abort();
  }

  /// Insert 1: insert one
  Student s1{1, "Li Ming 1", Gender::Male, 2024, "SE", 3.8};
  Student s2{2, "Li Ming 2", Gender::Male, 2024, "SE", 3.82};
  Student s3{3, "Li Ming 3", Gender::Female, 2024, "CS", 3.78};
  Student s4{4, "Liu Hua 1", Gender::Male, 2023, "SE", 3.98};
  Student s5{5, "Liu Hua 2", Gender::Female, 2023, "CS", 3.89};
  conn->insert(s1);
  conn->insert(s2);
  conn->insert(s3);
  conn->insert(s4);
  conn->insert(s5);

  /// Insert 2: insert many
  std::vector<Student> students;
  for (int i = 0; i < 10; i++) {
    Student s;
    s.id = 5 + i;
    s.name = "Che hen " + std::to_string(i);
    s.gender = rand() % 2 == 0 ? Gender::Female : Gender::Male;
    s.entry_year = 2023;
    s.major = rand() % 2 == 0 ? "CS" : "SE";
    s.gpa = 3.5 + (rand() % 10) * 0.05;
    students.push_back(s);
  }
  conn->insert(students);
}

int main(int argc, char *argv[]) {
  /// Init Web Server
  lynx::EventLoop loop;
  lynx::WebServer server(&loop);

  LOG_INFO << "start Web server";

  /// Setup server and pool
  server.start();
  initDb(server.pool());

  /// Register handler
  StudentController::init(server.pool());
  StudentController controller;
  controller.registerHandler(server);

  /// print route table
  server.printRouteTable();

  loop.loop();
}
