#include "lynx/db/pg_connection.h"

#include <cstdlib>
#include <tuple>
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

REFLECTION_TEMPLATE_WITH_NAME(Student, "student", id, name, gender, entry_year,
                              major, gpa)

int main() {
  /// Connect database
  lynx::PgConnection conn("PgConnection");
  conn.connect("127.0.0.1", "5432", "postgres", "123456", "demo");

  /// Create table (drop if table already exists)
  conn.execute("drop table student; drop sequence student_id_seq;");
  // lynx::KeyMap key_map{"id"};
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

  /// Query 1: query all
  auto result1 = conn.query<Student>().toVector();
  LOG_INFO << "query all";
  for (auto &res : result1) {
    std::cout << lynx::serialize(res) << "\n";
  }

  /// Query 2: query with condition
  auto result2 = conn.query<Student>()
                     .where(VALUE(Student::entry_year) == 2024 &&
                            VALUE(Student::major) == "CS")
                     .orderBy(VALUE(Student::gpa))
                     .toVector();
  LOG_INFO << "query with condition";
  for (auto &res : result2) {
    std::cout << lynx::serialize(res) << "\n";
  }

  /// Query 3: query some column
  auto result3 =
      conn.query<Student>()
          .select(FIELD(Student::id), FIELD(Student::name), FIELD(Student::gpa))
          .where(VALUE(Student::major) == "SE")
          .orderByDesc(VALUE(Student::gpa))
          .toVector();
  LOG_INFO << "query some column";
  for (auto it : result3) {
    std::apply([](auto &&...args) { ((std::cout << args << " "), ...); }, it);
    std::cout << "\n";
  }

  /// Query 4: query use group by
  auto result4 = conn.query<Student>()
                     .select(FIELD(Student::major), ORM_AVG(Student::gpa))
                     .groupBy(VALUE(Student::major))
                     .toVector();
  LOG_INFO << "query use group by";
  for (auto it : result4) {
    std::apply([](auto &&...args) { ((std::cout << args << " "), ...); }, it);
    std::cout << "\n";
  }

  /// Update
  auto flag1 = conn.update<Student>()
                   .set((VALUE(Student::major) = "AI") |
                        (VALUE(Student::entry_year) == 2025))
                   .where(VALUE(Student::gpa) >= 3.85)
                   .execute();
  LOG_INFO << "update " << (flag1 ? "success" : "fail");

  /// Query after update
  auto result5 =
      conn.query<Student>().orderByDesc(VALUE(Student::gpa)).toVector();
  LOG_INFO << "query all after update";
  for (auto &res : result5) {
    std::cout << lynx::serialize(res) << "\n";
  }

  /// Delete
  auto flag2 = conn.del<Student>().where(VALUE(Student::gpa) < 3.75).execute();
  LOG_INFO << "delete " << (flag2 ? "success" : "fail");

  /// Query after delete
  auto result6 =
      conn.query<Student>().orderByDesc(VALUE(Student::gpa)).toVector();
  LOG_INFO << "query all after delete";
  for (auto &res : result6) {
    std::cout << lynx::serialize(res) << "\n";
  }
}
