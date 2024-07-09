#ifndef LYNX_BASE_EXCEPTION_H
#define LYNX_BASE_EXCEPTION_H

#include <exception>
#include <string>

namespace lynx {

class Exception : public std::exception {
public:
  Exception(std::string msg);
  ~Exception() noexcept override = default;

  const char *what() const noexcept override { return message_.c_str(); }
  const char *stackTrace() const noexcept { return stack_.c_str(); }

private:
  std::string message_;
  std::string stack_;
};

} // namespace lynx

#endif
