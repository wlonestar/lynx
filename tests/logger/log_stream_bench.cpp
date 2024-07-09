#include "lynx/base/timestamp.h"
#include "lynx/logger/log_stream.h"

#include <cstdio>
#include <ios>
#include <sstream>

const size_t N = 1000000;

#pragma GCC diagnostic ignored "-Wold-style-cast"

template <typename T> void benchPrintf(const char *fmt) {
  char buf[32];
  lynx::Timestamp start(lynx::Timestamp::now());
  for (size_t i = 0; i < N; i++) {
    snprintf(buf, sizeof(buf), fmt, (T)(i));
  }
  lynx::Timestamp end(lynx::Timestamp::now());
  printf("benchPrintf %f\n", lynx::timeDifference(end, start));
}

template <typename T> void benchStringStream() {
  lynx::Timestamp start(lynx::Timestamp::now());
  std::ostringstream os;
  for (size_t i = 0; i < N; i++) {
    os << (T)(i);
    os.seekp(0, std::ios_base::beg);
  }
  lynx::Timestamp end(lynx::Timestamp::now());
  printf("benchStringStream %f\n", lynx::timeDifference(end, start));
}

template <typename T> void benchLogStream() {
  lynx::Timestamp start(lynx::Timestamp::now());
  lynx::LogStream os;
  for (size_t i = 0; i < N; i++) {
    os << (T)(i);
    os.resetBuffer();
  }
  lynx::Timestamp end(lynx::Timestamp::now());
  printf("benchLogStream %f\n", lynx::timeDifference(end, start));
}

int main() {
  benchPrintf<int>("%d");

  puts("int");
  benchPrintf<int>("%d");
  benchStringStream<int>();
  benchLogStream<int>();

  puts("double");
  benchPrintf<double>("%.12g");
  benchStringStream<double>();
  benchLogStream<double>();

  puts("int64_t");
  benchPrintf<int64_t>("%ld");
  benchStringStream<int64_t>();
  benchLogStream<int64_t>();

  puts("void*");
  benchPrintf<void *>("%p");
  benchStringStream<void *>();
  benchLogStream<void *>();
}
