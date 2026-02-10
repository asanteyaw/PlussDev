#include <MSharp/util/TypeCast.h>

namespace MSharp {

[[noreturn]] void report_overflow(const char* name) {
  std::ostringstream oss;
  oss << "value cannot be converted to type " << name << " without overflow";
  throw std::runtime_error(oss.str()); // rather than domain_error (issue 33562)
}

} // namespace MSharp
