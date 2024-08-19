#include "lynx/app/application.h"
#include "lynx/web/base_controller.h"
#include "lynx/web/base_repository.h"
#include "lynx/web/common_result.h"

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
  auto conn = pool.acquire();
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

class StudentRepository : public lynx::BaseRepository<Student, uint64_t> {
public:
  explicit StudentRepository(lynx::ConnectionPool &pool)
      : lynx::BaseRepository<Student, uint64_t>(pool) {}

  std::vector<Student> selectAll() {
    auto conn = pool_.acquire();
    auto students = conn->query<Student, uint64_t>().toVector();
    return students;
  }
};

class StudentService {
public:
  explicit StudentService(const StudentRepository &repository)
      : repository_(repository) {}

  std::vector<Student> selectTop100() { return repository_.selectTop100(); }
  std::vector<Student> selectAll() { return repository_.selectAll(); }

  std::vector<Student> selectByPage(size_t page, size_t size) {
    return repository_.selectByPage(page, size);
  }

  std::optional<Student> selectById(uint64_t id) {
    return repository_.selectById(id);
  }

  bool insert(Student student) { return repository_.insert(student) == 1; }

  int insert(std::vector<Student> students) {
    return repository_.insert(students);
  }

  bool updateById(uint64_t id, Student &&student) {
    return repository_.updateById(id, std::move(student));
  }

  bool deleteById(uint64_t id) { return repository_.delById(id); }

private:
  StudentRepository repository_;
};

const static std::string ID_REX = R"(\d+)";

class StudentController : public lynx::BaseController {
public:
  static void init(lynx::ConnectionPool &pool) {
    service = std::make_unique<StudentService>(StudentRepository(pool));
  }

  explicit StudentController() {
    if (service == nullptr) {
      LOG_FATAL << "Please init controller first";
      return;
    }
    requestMapping("GET", "/student100", selectTop100);
    requestMapping("GET", "/student", selectAll);
    requestMapping("GET", "/student\\?page=" + ID_REX + "&size=" + ID_REX,
                   selectByPage, lynx::RequestParam<size_t>("page"),
                   lynx::RequestParam<size_t>("size"));
    requestMapping("GET", "/student/" + ID_REX, selectById,
                   lynx::PathVariable<uint64_t>());
    requestMapping("POST", "/student", insert, lynx::RequestBody<Student>());
    requestMapping("PUT", "/student/" + ID_REX, updateById,
                   lynx::PathVariable<uint64_t>(),
                   lynx::RequestBody<Student>());
    requestMapping("DELETE", "/student/" + ID_REX, deleteById,
                   lynx::PathVariable<uint64_t>());
  }

  /// "GET" "/student100"
  static lynx::json selectTop100() {
    return lynx::makeOkResult("query success", service->selectTop100());
  }

  /// "GET" "/student"
  static lynx::json selectAll() {
    return lynx::makeOkResult("query success", service->selectAll());
  }

  /// "GET" "/student?page={page}&size={size}"
  static lynx::json selectByPage(size_t page, size_t size) {
    return lynx::makeOkResult("query success",
                              service->selectByPage(page, size));
  }

  /// "GET" "/student/{id}"
  static lynx::json selectById(uint64_t id) {
    if (auto data = service->selectById(id)) {
      return lynx::makeOkResult("query success", *data);
    }
    return lynx::makeErrorResult("query fail", "id not exists");
  }

  /// "POST" "/student",
  static lynx::json insert(Student student) {
    if (service->insert(student)) {
      return lynx::makeOkResult("insert success", student);
    }
    return lynx::makeErrorResult("insert fail", "id not exists");
  }

  /// "PUT" "/student/{id}"
  static lynx::json updateById(uint64_t id, Student &student) {
    if (service->updateById(id, std::move(student))) {
      return lynx::makeOkResult<std::string>("update success", "success");
    }
    return lynx::makeErrorResult("update fail", "id not exists");
  }

  /// "DELETE" "/student/{id}"
  static lynx::json deleteById(uint64_t id) {
    if (service->deleteById(id)) {
      return lynx::makeOkResult<std::string>("delete success", "success");
    }
    return lynx::makeErrorResult("delete fail", "id not exists");
  }

private:
  static std::unique_ptr<StudentService> service;
};

std::unique_ptr<StudentService> StudentController::service;

int main() {
  /// Create app by reading from config file.
  lynx::Application app("simple_config_3.yml");
  /// Init app.
  app.start();

  /// Register handlers.
  StudentController::init(app.pool());
  StudentController controller;
  controller.registerHandler(app);

  /// Start listening.
  app.listen();
}
