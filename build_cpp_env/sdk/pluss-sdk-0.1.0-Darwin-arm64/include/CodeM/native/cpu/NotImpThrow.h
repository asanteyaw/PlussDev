#include <stdexcept>
#include <string>

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::cpu {
  template <class Ret, class... Args>
  Ret unimplemented(std::string_view op, Args&&...) {
    ::pluss::throw_error(std::string("native::cpu op not wired: ") + std::string(op));
  }
  template <class... Args>
  void unimplemented_void(std::string_view op, Args&&...) {
    ::pluss::throw_error(std::string("native::cpu op not wired: ") + std::string(op));
  }
} // namespace pluss::codem::native::cpu
