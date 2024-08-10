# Lynx: A Multi-Thread Linux Server Framework

A flexible and scalable server framework for Linux written in C++ with support for multi-threading.

## Features

1. **Network Library**: Implement a network library leveraging the Reactor pattern for non-blocking I/O operations and event-driven programming, drawing inspiration from the [muduo](https://github.com/chenshuo/muduo) network library.
2. **HTTP Server**: Develops an HTTP server that incorporates Ragel, a powerful finite state machine compiler, for parsing HTTP requests.
3. **ORM framework**: Leverage advanced C++ techniques such as macros and template metaprogramming to implement compile-time reflection, which is used to encapsulate a ORM framework.

## Requirements

- Clang
- CMake
- Ninja
- Ragel
- Boost
- PostgreSQL
- nlohmann/json
- yaml-cpp

Ubuntu/Debian:

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
cmake -G Ninja -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

You need to set the compiler to clang

3. The static library `liblynx.a` will be created in `build/lib` directory.

## Usage

To integrate lynx into your CMake project, use `find_package` to include header files and link the library.

1. Set the proper install prefix:

```bash
cmake -G Ninja -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_INSTALL_PREFIX=<installation_path> \
  -DCMAKE_BUILD_TYPE=Release
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
target_link_libraries(app
  lynx pq nlohmann_json::nlohmann_json yaml-cpp)
```

4. Create a simpliest application.

```cpp
int main() {
  /// Init app.
  lynx::Application app;
  /// Start app.
  app.start();
  /// Start listening.
  app.listen();
}
```

After building, now you can access `127.0.0.1:8000/` to see the result like below.

![](https://image-1305118058.cos.ap-nanjing.myqcloud.com/image/Snipaste_2024-08-10_21-30-57.jpg)

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
