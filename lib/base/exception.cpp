#include "lynx/base/exception.h"
#include "lynx/base/current_thread.h"

namespace lynx {

Exception::Exception(std::string msg)
    : message_(std::move(msg)),
      stack_(current_thread::stackTrace(/*demangle=*/false)) {}

} // namespace lynx
