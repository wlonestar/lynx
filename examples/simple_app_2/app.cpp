#include "lynx/app/application.h"

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
                              major, gpa);
REGISTER_AUTO_KEY(Student, id);

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
  /// Insert data
  std::vector<Student> students;
  for (int i = 0; i < 20; i++) {
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

int main() {
  /// Create app by reading from config file.
  lynx::Application app("simple_config_2.yml");
  /// Init app.
  app.start();

  /// Init database
  initDb(app.pool());

  /// Add route.
  app.addRoute("GET", "/student", [&](auto &req, lynx::HttpResponse *resp) {
    auto conn = app.pool().getConnection();
    // Auto convert to json
    auto data = conn->query<Student, uint64_t>().toVector();
    lynx::json result;
    result["status"] = 200;
    result["message"] = "query succes";
    result["data"] = data;

    resp->setStatusCode(lynx::HttpStatus::OK);
    resp->setContentType("application/json");
    resp->addHeader("Server", "lynx");
    resp->setBody(result.dump()); /// json to string
  });

  /// Start listening.
  app.listen();
}
