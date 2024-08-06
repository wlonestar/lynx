# ORM 框架

利用 C++ 宏魔法和模板元编程实现编译期反射，再借助 `nlohmann/json` 实现 JSON 序列化和反序列化。对需要反射的类对象使用宏，从而获取数据库表对应的 C++ 类结构的各成员变量名称及其类型和值，使得 CRUD 的 SQL 语句可以自动生成。

```cpp
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
```

使用 `libpq` 库将 PostgreSQL API 封装成类，分别针对插入、查询、修改、删除等操作编写 SQL 代码生成函数以实现链式查询操作。

```cpp
auto result = conn.query<Student, uint64_t>()
                 .where(VALUE(Student::entry_year) == 2024 &&
                        VALUE(Student::major) == "CS")
                 .orderBy(VALUE(Student::gpa))
                 .toVector();
for (auto &res : result) {
  std::cout << lynx::serialize(res) << "\n";
}
```
