# Lynx: A Scalable, Multi-Threaded C++ Server Framework for Linux

Lynx is a flexible and scalable server framework for Linux written in C++ with support for multi-threading and non-blocking I/O.

## Features

1. **Network Library**: Implements a network library leveraging the Reactor pattern for non-blocking I/O operations and event-driven programming, drawing inspiration from the muduo network library.
2. **HTTP Server**: Develops an HTTP server that incorporates Ragel, a powerful finite state machine compiler, for parsing HTTP requests.
3. **ORM Framework**: Utilizes advanced C++ techniques such as macros and template metaprogramming to implement compile-time reflection, encapsulating an ORM framework.
4. **Auto Register Handler**: Provides automatic registration of HTTP handlers similar to Spring Boot’s @RequestMapping annotation.

## Requirements

- Clang
- CMake
- Ninja
- Ragel
- Boost
- PostgreSQL
- nlohmann/json
- yaml-cpp

Ubuntu/Debian Installation:

```bash
sudo apt install clang cmake ninja-build ragal libboost-test-dev libpq-dev nlohmann-json3-dev libyaml-cpp-dev
```

## Installation

1. Clone the repository:

```bash
git clone https://github.com/wlonestar/lynx.git && cd lynx
```

2. Configure and build the project using CMake:

```bash
cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

Ensure you set the compiler to clang.

3. The static library `liblynx.a` will be created in the `build/lib` directory.

## Usage

To integrate lynx into your CMake project, use `find_package` to include header files and link the library.

1. Set the proper install prefix:

```bash
cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=<installation_path> -DCMAKE_BUILD_TYPE=Release
ninja -C build
ninja -C build install
```

2. Use `find_package` in your CMake project, such as:

```cmake
set(CMAKE_PREFIX_PATH <installation_path>)
find_package(lynx CONFIG REQUIRED)
include_directories(${lynx_INCLUDE_DIRS})
```

3. Link the third party libraries that lynx uses:

```cmake
add_executable(app main.cpp)
target_link_libraries(app lynx pq nlohmann_json::nlohmann_json yaml-cpp)
```

4. Create a simply application:

```cpp
int main() {
  /// Create app.
  lynx::Application app;
  /// Init app.
  app.start();
  /// Start listening.
  app.listen();
}
```

After building, you can access `127.0.0.1:8000/` to see the result like below.

![](https://image-1305118058.cos.ap-nanjing.myqcloud.com/image/Snipaste_2024-08-10_21-30-57.jpg)

## API Usage Examples

This section provides examples of how to use various components of the Lynx framework. You can access the source code in the `examples` directory.

### Example 0: Setting Up an Application Server

Create a basic application server without any handler.

```cpp
int main() {
  /// Create app.
  lynx::Application app;
  /// Init app.
  app.start();
  /// Start listening.
  app.listen();
}
```

### Example 1: Parsing Param in HTTP Requests

Create a basic application server that responds with a simple message.

```cpp
void handleIndex(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpStatus::OK);
  resp->setContentType("text/html");
  resp->addHeader("Server", "lynx");
  std::string now = lynx::Timestamp::now().toFormattedString(true);
  resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " +
                now + "</body></html>");
}

void handleFavicon(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpStatus::OK);
  resp->setContentType("image/png");
  resp->setBody(
      std::string(reinterpret_cast<char *>(favicon_jpg), favicon_jpg_len));
}

int main() {
  /// Init Async logger.
  off_t roll_size = 500 * 1000 * 1000;
  char name[256] = {'\0'};
  strncpy(name, argv[0], sizeof(name) - 1);
  lynx::AsyncLogging log(::basename(name), roll_size);
  log.start();
  lynx::Logger::setOutput(
      [&](const char *msg, int len) { log.append(msg, len); });

  /// Create app by reading from config file.
  lynx::Application app("simple_config_1.yml");
  /// Init app.
  app.start();

  /// Add route.
  app.addRoute("GET", "/", handleIndex);
  app.addRoute("GET", "/favicon.ico", handleFavicon);
  app.addRoute("GET", "/hello\\?name=(\\w+)",
               [](const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
                 auto name = req.getParam("name");
                 resp->setStatusCode(lynx::HttpStatus::OK);
                 resp->setContentType("text/html");
                 resp->addHeader("Server", "lynx");
                 std::string now =
                     lynx::Timestamp::now().toFormattedString(true);
                 resp->setBody("<h1>Hello " + name + "!</h1>");
               });

  /// Start listening.
  app.listen();
}
```

### Example 2: Using Database with PostgreSQL

Interact with a PostgreSQL database using Lynx.

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
                              major, gpa);
REGISTER_AUTO_KEY(Student, id);

void initDb(lynx::ConnectionPool &pool) {
  auto conn = pool.getConnection();
  /// Create table (drop if table already exists)
  conn->execute("drop table student; drop sequence student_id_seq;");
  lynx::AutoKeyMap key_map{"id"};
  lynx::NotNullMap not_null_map;
  not_null_map.fields = {"id", "name", "gender", "entry_year"};
  bool flag = conn->createTable<Student>(key_map, not_null_map);
  if (!flag) {
    abort();
  }
  /// Insert data
  std::vector<Student> students;
  for (int i = 0; i < 20; i++) {
    /// ...
  }
  conn->insert(students);
}

int main() {
  /// Create app by reading from config file.
  lynx::Application app("simple_config_2.yml");
  /// Init app.
  app.start();

  /// Init database
  initDb(app.pool());

  /// Add route.
  app.addRoute("GET", "/student", [&](auto &req, lynx::HttpResponse *resp) {
    auto conn = app.pool().getConnection();
    // Auto convert to json
    auto data = conn->query<Student, uint64_t>().toVector();
    lynx::json result;
    result["status"] = 200;
    result["message"] = "query succes";
    result["data"] = data;

    resp->setStatusCode(lynx::HttpStatus::OK);
    resp->setContentType("application/json");
    resp->addHeader("Server", "lynx");
    resp->setBody(result.dump()); /// json to string
  });

  /// Start listening.
  app.listen();
}
```

### Example 3: Using Web Framework to automatically register HTTP handlers

```cpp
class StudentRepository : public lynx::BaseRepository<Student, uint64_t> {
public:
  explicit StudentRepository(lynx::ConnectionPool &pool) : lynx::BaseRepository<Student, uint64_t>(pool) {}

  std::vector<Student> selectAll() {
    auto conn = pool_.getConnection();
    auto students = conn->query<Student, uint64_t>().toVector();
    return students;
  }
};

class StudentService {
public:
  explicit StudentService(const StudentRepository &repository) : repository_(repository) {}

  std::vector<Student> selectTop100() { return repository_.selectTop100(); }
  std::vector<Student> selectAll() { return repository_.selectAll(); }
  // ...

private:
  StudentRepository repository_;
};

const static std::string ID_REX = R"(\d+)";

class StudentController : public lynx::BaseController {
public:
  static void init(lynx::ConnectionPool &pool) {
    service = std::make_unique<StudentService>(StudentRepository(pool));
  }

  explicit StudentController() {
    if (service == nullptr) {
      LOG_FATAL << "Please init controller first";
      return;
    }
    requestMapping("GET", "/student100", selectTop100);
    requestMapping("GET", "/student", selectAll);
    requestMapping("GET", "/student\\?page=" + ID_REX + "&size=" + ID_REX,
                   selectByPage, lynx::RequestParam<size_t>("page"),
                   lynx::RequestParam<size_t>("size"));
    requestMapping("GET", "/student/" + ID_REX, selectById,
                   lynx::PathVariable<uint64_t>());
    requestMapping("POST", "/student", insert, lynx::RequestBody<Student>());
    requestMapping("PUT", "/student/" + ID_REX, updateById,
                   lynx::PathVariable<uint64_t>(),
                   lynx::RequestBody<Student>());
    requestMapping("DELETE", "/student/" + ID_REX, deleteById,
                   lynx::PathVariable<uint64_t>());
  }

  /// "GET" "/student100"
  static lynx::json selectTop100() {
    return lynx::makeOkResult("query success", service->selectTop100());
  }

  /// "GET" "/student"
  static lynx::json selectAll() {
    return lynx::makeOkResult("query success", service->selectAll());
  }

  /// "GET" "/student?page={page}&size={size}"
  static lynx::json selectByPage(size_t page, size_t size) {
    return lynx::makeOkResult("query success",
                              service->selectByPage(page, size));
  }

  /// "GET" "/student/{id}"
  static lynx::json selectById(uint64_t id) {
    if (auto data = service->selectById(id)) {
      return lynx::makeOkResult("query success", *data);
    }
    return lynx::makeErrorResult("query fail", "id not exists");
  }

  /// "POST" "/student",
  static lynx::json insert(Student student) {
    if (service->insert(student)) {
      return lynx::makeOkResult("insert success", student);
    }
    return lynx::makeErrorResult("insert fail", "id not exists");
  }

  /// "PUT" "/student/{id}"
  static lynx::json updateById(uint64_t id, Student &student) {
    if (service->updateById(id, std::move(student))) {
      return lynx::makeOkResult<std::string>("update success", "success");
    }
    return lynx::makeErrorResult("update fail", "id not exists");
  }

  /// "DELETE" "/student/{id}"
  static lynx::json deleteById(uint64_t id) {
    if (service->deleteById(id)) {
      return lynx::makeOkResult<std::string>("delete success", "success");
    }
    return lynx::makeErrorResult("delete fail", "id not exists");
  }

private:
  static std::unique_ptr<StudentService> service;
};

std::unique_ptr<StudentService> StudentController::service;

int main() {
  /// Create app by reading from config file.
  lynx::Application app("simple_config_3.yml");
  /// Init app.
  app.start();

  /// Register handlers.
  StudentController::init(app.pool());
  StudentController controller;
  controller.registerHandler(app);

  /// Start listening.
  app.listen();
}
```

## Contributing

Contributions to the framework are welcome! Please follow these guidelines:

- Fork the repository and create a new branch for your changes.
- Write tests for your changes.
- Submit a pull request with a clear description of your changes.

## License

Lynx is licensed under the [MIT License](https://github.com/wlonestar/lynx/blob/master/LICENSE).

## References

[1] https://github.com/chenshuo/muduo

[2] https://github.com/hanson-young/orm-cpp

[3] https://github.com/mongrel2/mongrel2

[4] https://github.com/qicosmos/ormpp

[5] https://github.com/Shangyizhou/A-Tiny-Network-Library
