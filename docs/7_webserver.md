# Web 框架

参考 Spring Boot Repository，利用基类 `BaseRepository` 提供更高层次封装。

```cpp
template <typename T, typename ID> class BaseRepository {
public:
  explicit BaseRepository(lynx::PgConnectionPool &pool);

  virtual std::vector<T> selectByPage(size_t page, size_t size);
  virtual std::optional<T> selectById(ID id);
  virtual int insert(T &t);
  virtual int insert(std::vector<T> &t);
  virtual bool updateById(ID id, T &&t);
  virtual bool delById(ID id);

protected:
  lynx::PgConnectionPool &pool_;
};
```

参考 Spring Boot Rest Controller，在基类 `BaseController` 中编写不同类型 HTTP 请求的解析映射，实现 HTTP Handler 的自动注册。

```cpp
class StudentController : public lynx::BaseController {
public:
  static void init(lynx::PgConnectionPool &pool) {
    service = std::make_unique<StudentService>(StudentRepository(pool));
  }

  explicit StudentController() {
    requestMapping("GET", "/student", selectAll);
    requestMapping("GET", R"(/student\?page=(\d+)&size=(\d+))", selectByPage,
                   lynx::RequestParam<size_t>("page"),
                   lynx::RequestParam<size_t>("size"));
    requestMapping("GET", R"(/student/(\d+))", selectById,
                   lynx::PathVariable<uint64_t>());
    requestMapping("POST", "/student", insert, lynx::RequestBody<Student>());
    requestMapping("PUT", R"(/student/(\d+))", updateById,
                   lynx::PathVariable<uint64_t>(),
                   lynx::RequestBody<Student>());
    requestMapping("DELETE", R"(/student/(\d+))", deleteById,
                   lynx::PathVariable<uint64_t>());
  }

  /// "GET" "/student"
  static lynx::json selectAll();
  /// "GET" "/student?page=(\d+)&size=(\d+)"
  static lynx::json selectByPage(size_t page, size_t size);
  /// "GET" "/student/(\d+)"
  static lynx::json selectById(uint64_t id);
  /// "POST" "/student",
  static lynx::json insert(Student student);
  /// "PUT" "/student/(\d+)"
  static lynx::json updateById(uint64_t id, Student &student);
  /// "DELETE" "/student/(\d+)"
  static lynx::json deleteById(uint64_t id);

private:
  static std::unique_ptr<StudentService> service;
};
```

应用服务器（WebServer）维护一个路由表，路由表记录着请求方法、请求路径及其对应的 Handler，通过匹配请求方法和请求路径调用对应的 Handler。

应用服务器通过 yaml 格式文件读取 HTTP 服务器和数据库连接池等相关配置信息。
