#include "lynx/app/Application.h"
#include "lynx/web/BaseController.h"
#include "lynx/web/BaseRepository.h"
#include "lynx/web/CommonResult.h"

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

class StudentRepository : public lynx::BaseRepository<Student, uint64_t> {
public:
  explicit StudentRepository(lynx::ConnectionPool &Pool)
      : lynx::BaseRepository<Student, uint64_t>(Pool) {}

  std::vector<Student> selectAll() {
    auto Conn = Pool.acquire();
    auto Students = Conn->query<Student, uint64_t>().toVector();
    return Students;
  }
};

class StudentService {
public:
  explicit StudentService(const StudentRepository &Repository)
      : Repository(Repository) {}

  std::vector<Student> selectTop100() { return Repository.selectTop100(); }
  std::vector<Student> selectAll() { return Repository.selectAll(); }

  std::vector<Student> selectByPage(size_t Page, size_t Size) {
    return Repository.selectByPage(Page, Size);
  }

  std::optional<Student> selectById(uint64_t Id) {
    return Repository.selectById(Id);
  }

  bool insert(Student Student) { return Repository.insert(Student) == 1; }

  int insert(std::vector<Student> Students) {
    return Repository.insert(Students);
  }

  bool updateById(uint64_t Id, Student &&Student) {
    return Repository.updateById(Id, std::move(Student));
  }

  bool deleteById(uint64_t Id) { return Repository.delById(Id); }

private:
  StudentRepository Repository;
};

const static std::string IdRex = R"(\d+)";

class StudentController : public lynx::BaseController {
public:
  static void init(lynx::ConnectionPool &Pool) {
    Service = std::make_unique<StudentService>(StudentRepository(Pool));
  }

  explicit StudentController() {
    if (Service == nullptr) {
      LOG_FATAL << "Please init controller first";
      return;
    }
    requestMapping("GET", "/student100", selectTop100);
    requestMapping("GET", "/student", selectAll);
    requestMapping("GET", "/student\\?page=" + IdRex + "&size=" + IdRex,
                   selectByPage, lynx::RequestParam<size_t>("page"),
                   lynx::RequestParam<size_t>("size"));
    requestMapping("GET", "/student/" + IdRex, selectById,
                   lynx::PathVariable<uint64_t>());
    requestMapping("POST", "/student", insert, lynx::RequestBody<Student>());
    requestMapping("PUT", "/student/" + IdRex, updateById,
                   lynx::PathVariable<uint64_t>(),
                   lynx::RequestBody<Student>());
    requestMapping("DELETE", "/student/" + IdRex, deleteById,
                   lynx::PathVariable<uint64_t>());
  }

  /// "GET" "/student100"
  static lynx::json selectTop100() {
    return lynx::makeOkResult("query success", Service->selectTop100());
  }

  /// "GET" "/student"
  static lynx::json selectAll() {
    return lynx::makeOkResult("query success", Service->selectAll());
  }

  /// "GET" "/student?page={page}&size={size}"
  static lynx::json selectByPage(size_t Page, size_t Size) {
    return lynx::makeOkResult("query success",
                              Service->selectByPage(Page, Size));
  }

  /// "GET" "/student/{id}"
  static lynx::json selectById(uint64_t Id) {
    if (auto Data = Service->selectById(Id)) {
      return lynx::makeOkResult("query success", *Data);
    }
    return lynx::makeErrorResult("query fail", "id not exists");
  }

  /// "POST" "/student",
  static lynx::json insert(Student Student) {
    if (Service->insert(Student)) {
      return lynx::makeOkResult("insert success", Student);
    }
    return lynx::makeErrorResult("insert fail", "id not exists");
  }

  /// "PUT" "/student/{id}"
  static lynx::json updateById(uint64_t Id, Student &Student) {
    if (Service->updateById(Id, std::move(Student))) {
      return lynx::makeOkResult<std::string>("update success", "success");
    }
    return lynx::makeErrorResult("update fail", "id not exists");
  }

  /// "DELETE" "/student/{id}"
  static lynx::json deleteById(uint64_t Id) {
    if (Service->deleteById(Id)) {
      return lynx::makeOkResult<std::string>("delete success", "success");
    }
    return lynx::makeErrorResult("delete fail", "id not exists");
  }

private:
  static std::unique_ptr<StudentService> Service;
};

std::unique_ptr<StudentService> StudentController::Service;

int main() {
  /// Create app by reading from config file.
  lynx::Application App("simple_config_3.yml");
  /// Init app.
  App.start();

  /// Register handlers.
  StudentController::init(App.pool());
  StudentController Controller;
  Controller.registerHandler(App);

  /// Start listening.
  App.listen();
}
