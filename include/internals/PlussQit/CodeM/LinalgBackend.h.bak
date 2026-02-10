#pragma once

#include <MSharp/util/Exception.h>

#include <ostream>
#include <string>

namespace pluss {

enum class LinalgBackend : int8_t { Default, Cusolver, Magma };

inline std::string LinalgBackendToString(pluss::LinalgBackend backend) {
  switch (backend) {
    case LinalgBackend::Default:
      return "pluss::LinalgBackend::Default";
    case LinalgBackend::Cusolver:
      return "pluss::LinalgBackend::Cusolver";
    case LinalgBackend::Magma:
      return "pluss::LinalgBackend::Magma";
    default:
      TORCH_CHECK(false, "Unknown linalg backend");
  }
}

inline std::ostream& operator<<(
    std::ostream& stream,
    pluss::LinalgBackend backend) {
  return stream << LinalgBackendToString(backend);
}

} // namespace pluss
