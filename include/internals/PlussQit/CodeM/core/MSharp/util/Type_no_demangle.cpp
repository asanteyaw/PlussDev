#include <MSharp/macros/Macros.h>

#if HAS_DEMANGLE == 0
#include <MSharp/util/Type.h>

namespace MSharp {
std::string demangle(const char* name) {
  return std::string(name);
}
} // namespace MSharp
#endif // !HAS_DEMANGLE
