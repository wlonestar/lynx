#include "lynx/db/pg_connection_pool.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/logger/logging.h"
#include "lynx/reflection.h"
#include "lynx/web/base_repository.h"
#include "lynx/web/base_rest_controller.h"

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

// NOLINTNEXTLINE
void from_json(const json &j, Student &s) {
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
  StudentRepository(lynx::PgConnectionPool &pool)
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
  StudentService(StudentRepository &repository) : repository_(repository) {}

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

template <typename T> // NOLINTNEXTLINE
void from_json(const json &j, Result<T> &result) {
  j.at("status").get_to(result.status);
  j.at("message").get_to(result.message);
  j.at("data").get_to(result.data);
}

class StudentController : public lynx::BaseController<StudentController> {
public:
  explicit StudentController(StudentService &service) : service_(service) {}

  /// @Method: GET
  /// @Path: /student100
  json selectTop100() {
    auto data = service_.selectTop100();
    Result<std::vector<Student>> result{200, "query success", data};
    json j = result;
    return j;
  }

  /// @Method: GET
  /// @Path: /student
  json selectAll() {
    auto data = service_.selectAll();
    Result<std::vector<Student>> result{200, "query success", data};
    json j = result;
    return j;
  }

  /// @Method: GET
  /// @Path: /student/<id>
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

  /// @Method: POST
  /// @Path: /student
  json insert(Student student) {
    if (service_.insert(student)) {
      Result<Student> result{200, "insert success", student};
      json j = result;
      return j;
    }
    Result<std::string> result{400, "insert fail", "id not exists"};
    json j = result;
    return j;
  }

  /// @Method: PUT
  /// @Path: /student/<id>
  json updateById(uint64_t id, Student &&student) {
    if (service_.updateById(id, std::move(student))) {
      Result<std::string> result{200, "update success", "success"};
      json j = result;
      return j;
    }
    Result<std::string> result{400, "update fail", "id not exists"};
    json j = result;
    return j;
  }

  /// @Method: DELETE
  /// @Path: /student/<id>
  json deleteById(uint64_t id) {
    if (service_.deleteById(id)) {
      Result<std::string> result{200, "delete success", "success"};
      json j = result;
      return j;
    }
    Result<std::string> result{400, "delete fail", "id not exists"};
    json j = result;
    return j;
  }

  void handleSelectTop100(const lynx::HttpRequest &req,
                          lynx::HttpResponse *resp) {
    setRespOk(resp);
    resp->setBody(selectTop100().dump());
  }

  void handleSelectAll(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
    setRespOk(resp);
    resp->setBody(selectAll().dump());
  }

  void handleSelectById(const lynx::HttpRequest &req,
                        lynx::HttpResponse *resp) {
    auto &path = req.path();
    uint64_t id = atoll(path.substr(path.find_last_of('/') + 1).c_str());
    setRespOk(resp);
    resp->setBody(selectById(id).dump());
  }

  void handleInsert(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
    auto &body = req.body();
    json j = json::parse(body);
    Student student = j;
    setRespOk(resp);
    resp->setBody(insert(student).dump());
  }

  void handleUpdateById(const lynx::HttpRequest &req,
                        lynx::HttpResponse *resp) {
    auto &body = req.body();
    json j = json::parse(body);
    Student student = j;

    auto &path = req.path();
    uint64_t id = atoll(path.substr(path.find_last_of('/') + 1).c_str());

    setRespOk(resp);
    resp->setBody(updateById(id, std::move(student)).dump());
  }

  void handleDeleteById(const lynx::HttpRequest &req,
                        lynx::HttpResponse *resp) {
    auto &path = req.path();
    uint64_t id = atoll(path.substr(path.find_last_of('/') + 1).c_str());

    setRespOk(resp);
    resp->setBody(deleteById(id).dump());
  }

  void assign(lynx::WebServer &server, StudentController &controller) {
    server.addRoute(
        "GET", "/student100", [&controller](auto &&PH1, auto &&PH2) {
          controller.handleSelectTop100(std::forward<decltype(PH1)>(PH1),
                                        std::forward<decltype(PH2)>(PH2));
        });
    server.addRoute("GET", "/student", [&controller](auto &&PH1, auto &&PH2) {
      controller.handleSelectAll(std::forward<decltype(PH1)>(PH1),
                                 std::forward<decltype(PH2)>(PH2));
    });
    server.addRoute(
        "GET", "/student/([0-9]+)", [&controller](auto &&PH1, auto &&PH2) {
          controller.handleSelectById(std::forward<decltype(PH1)>(PH1),
                                      std::forward<decltype(PH2)>(PH2));
        });
    server.addRoute("POST", "/student", [&controller](auto &&PH1, auto &&PH2) {
      controller.handleInsert(std::forward<decltype(PH1)>(PH1),
                              std::forward<decltype(PH2)>(PH2));
    });
    server.addRoute(
        "PUT", "/student/([0-9]+)", [&controller](auto &&PH1, auto &&PH2) {
          controller.handleUpdateById(std::forward<decltype(PH1)>(PH1),
                                      std::forward<decltype(PH2)>(PH2));
        });
    server.addRoute(
        "DELETE", "/student/([0-9]+)", [&controller](auto &&PH1, auto &&PH2) {
          controller.handleDeleteById(std::forward<decltype(PH1)>(PH1),
                                      std::forward<decltype(PH2)>(PH2));
        });
  }

private:
  StudentService service_;
};
