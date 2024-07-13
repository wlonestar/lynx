#include "lynx/db/pg_connection.h"

#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

enum Gender : int {
  Mail,
  Femail,
};

struct Person {
  short id;      // NOLINT
  char name[10]; // NOLINT
  Gender gender; // NOLINT
  int age;       // NOLINT
  float score;   // NOLINT
} __attribute__((packed));

REFLECTION_TEMPLATE_WITH_NAME(Person, "person", id, name, gender, age, score)

int main() {
  std::cout << std::boolalpha;

  // connect database
  lynx::PgConnection conn("PgConn");
  conn.connect("127.0.0.1", "5432", "postgres", "123456", "demo");
  // delete table
  conn.execute("drop table person;");
  // create table
  lynx::KeyMap key_map{"id"};
  lynx::NotNullMap not_null_map;
  not_null_map.fields = {"id", "age"};
  conn.createTable<Person>(key_map, not_null_map);

  // insert data
  Person p1{1, "hxf1", Gender::Femail, 30, 101.1F};
  Person p2{2, "hxf2", Gender::Femail, 28, 102.2F};
  Person p3{3, "hxf3", Gender::Mail, 27, 103.3F};
  Person p4{4, "hxf4", Gender::Femail, 26, 104.4F};
  Person p5{5, "hxf1", Gender::Mail, 30, 108.1F};
  Person p6{6, "hxf3", Gender::Femail, 30, 109.1F};

  conn.insert(p1);
  conn.insert(p2);
  conn.insert(p3);
  conn.insert(p4);
  conn.insert(p5);
  conn.insert(p6);

  std::vector<Person> persons;
  for (size_t i = 6; i < 10; i++) {
    Person p;
    p.id = i + 1;
    std::string name = "hxf" + std::to_string(i + 1);
    strncpy(p.name, name.c_str(), name.size());
    p.gender = Gender::Mail;
    p.age = 30 + i;
    p.score = 101.1F + i;
    persons.push_back(p);
  }
  conn.insert(persons);

  // query 1 return person struct
  auto pn1 = conn.query<Person>()
                 .where(VALUE(Person::age) > 27 && VALUE(Person::id) < 3)
                 .limit(2)
                 .toVector();

  for (auto it : pn1) {
    std::cout << it.id << " " << it.name << " " << it.gender << " " << it.age
              << " " << it.score << std::endl;
  }

  // query 2 return an array of tuple objects
  auto pn2 = conn.query<Person>()
                 .select(FIELD(Person::id), FIELD(Person::name),
                         FIELD(Person::gender), FIELD(Person::age))
                 .where(VALUE(Person::age) >= 28 && VALUE(Person::id) < 5)
                 .toVector();

  for (auto it : pn2) {
    std::apply([](auto &&...args) { ((std::cout << args << ' '), ...); }, it);
    std::cout << std::endl;
  }

  // query 3 return an array of tuple objects, use group by
  auto pn3 = conn.query<Person>()
                 .select(FIELD(Person::age), ORM_SUM(Person::score),
                         ORM_COUNT(Person::name))
                 .where(VALUE(Person::age) > 24 && VALUE(Person::id) < 7)
                 .limit(3)
                 .groupBy(VALUE(Person::age))
                 .orderByDesc(VALUE(Person::age))
                 .toVector();

  for (auto it : pn3) {
    std::apply([](auto &&...args) { ((std::cout << args << ' '), ...); }, it);
    std::cout << std::endl;
  }

  // update
  auto res2 =
      conn.update<Person>()
          .set((VALUE(Person::age) = 50) | (VALUE(Person::name) = "hxf100"))
          .where(VALUE(Person::age) > 29)
          .execute();
  (void)res2;

  // query all
  auto pn4 = conn.query<Person>().toVector();

  for (auto it : pn4) {
    std::cout << it.id << " " << it.name << " " << it.gender << " " << it.age
              << " " << it.score << std::endl;
  }

  // delete
  auto res = conn.del<Person>().where(VALUE(Person::age) > 29).execute();
  (void)res;

  return 0;
}
