#include "lynx/app/Application.h"

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

void initDb(lynx::ConnectionPool &Pool) {
  auto Conn = Pool.acquire();
  /// Create table (drop if table already exists)
  Conn->execute("drop table student; drop sequence student_id_seq;");
  lynx::AutoKeyMap KeyMap{"id"};
  lynx::NotNullMap NotNullMap;
  NotNullMap.fields = {"id", "name", "gender", "entry_year"};
  bool Flag = Conn->createTable<Student>(KeyMap, NotNullMap);
  if (!Flag) {
    abort();
  }
  /// Insert data
  std::vector<Student> Students;
  for (int I = 0; I < 20; I++) {
    Student S;
    S.id = 5 + I;
    S.name = "Che hen " + std::to_string(I);
    S.gender = rand() % 2 == 0 ? Gender::Female : Gender::Male;
    S.entry_year = 2023;
    S.major = rand() % 2 == 0 ? "CS" : "SE";
    S.gpa = 3.5 + (rand() % 10) * 0.05;
    Students.push_back(S);
  }
  Conn->insert(Students);
}

int main() {
  /// Create app by reading from config file.
  lynx::Application App("simple_config_2.yml");
  /// Init app.
  App.start();

  /// Init database
  initDb(App.pool());

  /// Add route.
  App.addRoute("GET", "/student", [&](auto &Req, auto *Resp) {
    auto Conn = App.pool().acquire();
    // Auto convert to json
    auto Data = Conn->query<Student, uint64_t>().toVector();
    lynx::json Result;
    Result["status"] = 200;
    Result["message"] = "query succes";
    Result["data"] = Data;

    Resp->setStatusCode(lynx::HttpStatus::OK);
    Resp->setContentType("application/json");
    Resp->addHeader("Server", "lynx");
    Resp->setBody(Result.dump()); /// json to string
  });

  /// Start listening.
  App.listen();
}
