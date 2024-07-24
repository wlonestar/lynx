#include "lynx/db/pg_connection_pool.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/logger/logging.h"
#include "lynx/reflection.h"
#include "lynx/web/base_repository.h"
#include "lynx/web/base_rest_controller.h"

#include <optional>
#include <vector>

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

// NOLINTNEXTLINE
void to_json(lynx::json &j, const Student &s) {
  j["id"] = s.id;
  j["name"] = s.name;
  j["gender"] = s.gender;
  j["entry_year"] = s.entry_year;
  j["major"] = s.major;
  j["gpa"] = s.gpa;
}

// NOLINTNEXTLINE
void from_json(const lynx::json &j, Student &s) {
  j.at("id").get_to(s.id);
  j.at("name").get_to(s.name);
  j.at("gender").get_to(s.gender);
  j.at("entry_year").get_to(s.entry_year);
  j.at("major").get_to(s.major);
  j.at("gpa").get_to(s.gpa);
}

REFLECTION_TEMPLATE_WITH_NAME(Student, "student", id, name, gender, entry_year,
                              major, gpa)
REGISTER_AUTO_KEY(Student, id)

class StudentRepository : public lynx::BaseRepository<Student, uint64_t> {
public:
  explicit StudentRepository(lynx::PgConnectionPool &pool)
      : lynx::BaseRepository<Student, uint64_t>(pool) {}

  std::vector<Student> selectAll() {
    auto conn = pool_.acquire();
    auto students = conn->query<Student, uint64_t>().toVector();
    pool_.release(conn);
    return students;
  }
};

class StudentService {
public:
  explicit StudentService(const StudentRepository &repository)
      : repository_(repository) {}

  std::vector<Student> selectTop100() { return repository_.selectTop100(); }
  std::vector<Student> selectAll() { return repository_.selectAll(); }

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

RestController(StudentController);
class StudentController {
public:
  explicit StudentController(const StudentService &service)
      : service_(service) {
    RegisterHandler(StudentController, "GET", "/student100", selectTop100);
    RegisterHandler(StudentController, "GET", "/student", selectAll);
    RegisterHandler(StudentController, "GET", "/student/([0-9]+)", selectById);
    RegisterHandler(StudentController, "POST", "/student", insert);
    RegisterHandler(StudentController, "PUT", "/student/([0-9]+)", updateById);
    RegisterHandler(StudentController, "DELETE", "/student/([0-9]+)",
                    deleteById);
  }

  RequestMapping(StudentController, "GET", "/student100", selectTop100);
  lynx::json selectTop100() {
    return lynx::makeOkResult("query success", service_.selectTop100());
  }

  RequestMapping(StudentController, "GET", "/student", selectAll);
  lynx::json selectAll() {
    return lynx::makeOkResult("query success", service_.selectAll());
  }

  RequestMappingWithPathVariable(StudentController, "GET", "/student/([0-9]+)",
                                 selectById, uint64_t);
  lynx::json selectById(uint64_t id) {
    if (auto data = service_.selectById(id)) {
      return lynx::makeOkResult("query success", *data);
    }
    return lynx::makeErrorResult("query fail", "id not exists");
  }

  RequestMappingWithBody(StudentController, "POST", "/student", insert,
                         Student);
  lynx::json insert(Student student) {
    if (service_.insert(student)) {
      return lynx::makeOkResult("insert success", student);
    }
    return lynx::makeErrorResult("insert fail", "id not exists");
  }

  RequestMappingWithPathVariableAndBody(StudentController, "PUT",
                                        "/student/([0-9]+)", updateById,
                                        uint64_t, Student);
  lynx::json updateById(uint64_t id, Student &student) {
    if (service_.updateById(id, std::move(student))) {
      return lynx::makeOkResult<std::string>("update success", "success");
    }
    return lynx::makeErrorResult("update fail", "id not exists");
  }

  RequestMappingWithPathVariable(StudentController, "DELETE",
                                 "/student/([0-9]+)", deleteById, uint64_t);
  lynx::json deleteById(uint64_t id) {
    if (service_.deleteById(id)) {
      return lynx::makeOkResult<std::string>("delete success", "success");
    }
    return lynx::makeErrorResult("delete fail", "id not exists");
  }

private:
  StudentService service_;
};
