#include "lynx/orm/pg_orm.h"

#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

enum Gender : int {
  Mail,
  Femail,
};

struct person {
  short id;
  char name[10];
  Gender gender;
  int age;
  float score;
} __attribute__((packed));
REFLECTION_TEMPLATE(person, id, name, gender, age, score)

enum class Color : int16_t { RED, GREEN, BLUE };

int main() {
  std::cout << std::boolalpha;

  // connect database
  lynx::PQconnection conn("127.0.0.1", "5432", "postgres", "123456", "demo");
  // delete table
  conn.execute("drop table person;");
  // create table
  lynx::KeyMap key_map_{"id"};
  lynx::NotNullMap not_null_map_;
  not_null_map_.fields = {"id", "age"};
  conn.create_table<person>(key_map_, not_null_map_);

  // insert data
  person p1{1, "hxf1", Gender::Femail, 30, 101.1F};
  person p2{2, "hxf2", Gender::Femail, 28, 102.2F};
  person p3{3, "hxf3", Gender::Mail, 27, 103.3F};
  person p4{4, "hxf4", Gender::Femail, 26, 104.4F};
  person p5{5, "hxf1", Gender::Mail, 30, 108.1F};
  person p6{6, "hxf3", Gender::Femail, 30, 109.1F};

  conn.insert(p1);
  conn.insert(p2);
  conn.insert(p3);
  conn.insert(p4);
  conn.insert(p5);
  conn.insert(p6);

  std::vector<person> persons;
  for (size_t i = 6; i < 10; i++) {
    person p;
    p.id = i + 1;
    std::string name = "hxf" + std::to_string(i + 1);
    strcpy(p.name, name.c_str());
    p.gender = Gender::Mail;
    p.age = 30 + i;
    p.score = 101.1F + i;
    persons.push_back(p);
  }
  conn.insert(persons);

  // query 1 return person struct
  auto pn1 = conn.query<person>()
                 .where(FD(person::age) > 27 && FD(person::id) < 3)
                 .limit(2)
                 .to_vector();

  for (auto it : pn1) {
    std::cout << it.id << " " << it.name << " " << it.gender << " " << it.age
              << " " << it.score << std::endl;
  }

  // query 2 return an array of tuple objects
  auto pn2 = conn.query<person>()
                 .select(RNT(person::id), RNT(person::name),
                         RNT(person::gender), RNT(person::age))
                 .where(FD(person::age) >= 28 && FD(person::id) < 5)
                 .to_vector();

  for (auto it : pn2) {
    std::apply([](auto &&...args) { ((std::cout << args << ' '), ...); }, it);
    std::cout << std::endl;
  }

  // query 3 return an array of tuple objects, use group by
  auto pn3 = conn.query<person>()
                 .select(RNT(person::age), ORM_SUM(person::score),
                         ORM_COUNT(person::name))
                 .where(FD(person::age) > 24 && FD(person::id) < 7)
                 .limit(3)
                 .group_by(FD(person::age))
                 .order_by_desc(FD(person::age))
                 .to_vector();

  for (auto it : pn3) {
    std::apply([](auto &&...args) { ((std::cout << args << ' '), ...); }, it);
    std::cout << std::endl;
  }

  // update
  auto res2 = conn.update<person>()
                  .set((FD(person::age) = 50) | (FD(person::name) = "hxf100"))
                  .where(FD(person::age) > 29)
                  .execute();
  (void)res2;

  // query all
  auto pn4 = conn.query<person>().to_vector();

  for (auto it : pn4) {
    std::cout << it.id << " " << it.name << " " << it.gender << " " << it.age
              << " " << it.score << std::endl;
  }

  // delete
  auto res = conn.del<person>().where(FD(person::age) > 29).execute();
  (void)res;

  return 0;
}
