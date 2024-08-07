#include "libpq-fe.h"
#include "lynx/base/thread.h"
#include "lynx/db/async_connection.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/orm/key_util.h"
#include "lynx/orm/reflection.h"

#include <cassert>
#include <cstdlib>
#include <tuple>
#include <vector>

PGresult *res;

template <typename Ty>
constexpr void assignValue(Ty &&value, int row, int col) {
  using U = std::remove_const_t<std::remove_reference_t<Ty>>;
  if constexpr (std::is_integral<U>::value &&
                !(std::is_same<U, int64_t>::value ||
                  std::is_same<U, uint64_t>::value)) {
    value = atoi(PQgetvalue(res, row, col));
  } else if constexpr (std::is_enum_v<U>) {
    value = static_cast<U>(atoi(PQgetvalue(res, row, col)));
  } else if constexpr (std::is_floating_point<U>::value) {
    value = atof(PQgetvalue(res, row, col));
  } else if constexpr (std::is_same<U, int64_t>::value ||
                       std::is_same<U, uint64_t>::value) {
    value = atoll(PQgetvalue(res, row, col));
  } else if constexpr (std::is_same<U, std::string>::value) {
    value = PQgetvalue(res, row, col);
  } else if constexpr (std::is_array<U>::value &&
                       std::is_same<char, std::remove_pointer_t<
                                              std::decay_t<U>>>::value) {
    auto ptr = PQgetvalue(res, row, col);
    memcpy(value, ptr, sizeof(U));
  } else {
    LOG_ERROR << "unsupported type:" << std::is_array<U>::value;
  }
}

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
                              major, gpa)
REGISTER_AUTO_KEY(Student, id)

void query(lynx::AsyncConnection &conn) {
  std::string query_sql = "select * from student;";
  LOG_INFO << "query: " << query_sql << " --> " << conn.query(query_sql);
  res = conn.getResult();
  assert(res != nullptr);
  LOG_INFO << res;

  std::vector<Student> ret_vector;

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    LOG_ERROR << PQresultErrorMessage(res);
    PQclear(res);
  }
  size_t ntuples = PQntuples(res);
  for (size_t i = 0; i < ntuples; i++) {
    Student tp = {};
    lynx::forEach(tp, [&tp, &i](auto item, auto field, auto j) {
      assignValue(tp.*item, i, static_cast<int>(decltype(j)::value));
    });
    ret_vector.push_back(std::move(tp));
  }
  PQclear(res);

  for (auto &student : ret_vector) {
    LOG_INFO << lynx::serialize(student);
  }
}

int main() {
  lynx::EventLoop loop;
  lynx::AsyncConnection conn(&loop);

  std::string connect_sql =
      "host=127.0.0.1 port=5432 user=postgres password=123456 dbname=demo";
  LOG_INFO << "connect: " << connect_sql << " --> "
           << conn.connect(connect_sql);

  lynx::Thread t1([&] {
    query(conn);
    query(conn);
    loop.quit();
  });
  t1.start();

  loop.loop();
}
