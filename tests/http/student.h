#include "lynx/web/base_repository.h"
#include <lynx/db/pg_connection_pool.h>
#include <lynx/http/http_request.h>
#include <lynx/http/http_response.h>
#include <lynx/orm/reflection.h>

#include <nlohmann/json.hpp>

#include <optional>
#include <vector>

using json = nlohmann::json;

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
void to_json(json &j, const Student &s) {
  j["id"] = s.id;
  j["name"] = s.name;
  j["gender"] = s.gender;
  j["entry_year"] = s.entry_year;
  j["major"] = s.major;
  j["gpa"] = s.gpa;
}

REFLECTION_TEMPLATE_WITH_NAME(Student, "student", id, name, gender, entry_year,
                              major, gpa)

class StudentRepository : public lynx::BaseRepository<Student, uint64_t> {
public:
  StudentRepository(lynx::PgConnectionPool &pool)
      : lynx::BaseRepository<Student, uint64_t>(pool) {}

  std::vector<Student> selectAll() {
    auto conn = pool_.acquire();
    auto students = conn->query<Student>().toVector();
    pool_.release(conn);
    return students;
  }

  std::optional<Student> selectById(uint64_t id) {
    auto conn = pool_.acquire();
    auto students =
        conn->query<Student>().where(VALUE(Student::id) == id).toVector();
    pool_.release(conn);
    if (students.empty()) {
      return std::nullopt;
    }
    return students[0];
  }
};

class StudentService {
public:
  StudentService(StudentRepository &repository) : repository_(repository) {}

  std::vector<Student> selectTop100() { return repository_.selectTop100(); }
  std::vector<Student> selectAll() { return repository_.selectAll(); }

  std::optional<Student> selectById(uint64_t id) {
    return repository_.selectById(id);
  }

  int insert(Student student) { return repository_.insert(student); }
  int insert(std::vector<Student> students) {
    return repository_.insert(students);
  }

private:
  StudentRepository repository_;
};

template <typename T> struct Result {
  int status;          // NOLINT
  std::string message; // NOLINT
  T data;              // NOLINT
};

template <typename T> // NOLINTNEXTLINE
void to_json(json &j, const Result<T> &result) {
  j["status"] = result.status;
  j["message"] = result.message;
  j["data"] = result.data;
}

class StudentController {
public:
  StudentController(StudentService &service) : service_(service) {}

  /// @Path: /student100
  json selectTop100() {
    auto data = service_.selectTop100();
    Result<std::vector<Student>> result{200, "query success", data};
    json j = result;
    return j;
  }

  /// @Path: /student
  json selectAll() {
    auto data = service_.selectAll();
    Result<std::vector<Student>> result{200, "query success", data};
    json j = result;
    return j;
  }

  /// @Path: /student/id?id=${id}
  json selectById(uint64_t id) {
    if (auto data = service_.selectById(id)) {
      Result<Student> result{200, "query success", *data};
      json j = result;
      return j;
    }
    Result<std::string> result{400, "query fail", "id not exists"};
    json j = result;
    return j;
  }

  void registe(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
    if (req.path() == "/student100") {
      resp->setStatusCode(lynx::HttpResponse::Ok200);
      resp->setStatusMessage("OK");
      resp->setContentType("application/json");
      resp->addHeader("Server", "lynx");
      resp->setBody(selectTop100().dump());
    }
    if (req.path() == "/student") {
      resp->setStatusCode(lynx::HttpResponse::Ok200);
      resp->setStatusMessage("OK");
      resp->setContentType("application/json");
      resp->addHeader("Server", "lynx");
      resp->setBody(selectAll().dump());
    }
    if (req.path() == "/student/id") {
      const auto &query = req.query();
      if (query.starts_with("?id=")) {
        resp->setStatusCode(lynx::HttpResponse::Ok200);
        resp->setStatusMessage("OK");
        resp->setContentType("application/json");
        resp->addHeader("Server", "lynx");
        uint64_t id = atoll(query.substr(4).c_str());
        resp->setBody(selectById(id).dump());
      }
    }
  }

private:
  StudentService service_;
};
