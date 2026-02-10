#pragma once

#include <MSharp/util/Exception.h>

#include <ostream>
#include <string>

namespace pluss {

enum class ROCmFABackend : int8_t { Default, AOTriton, Ck };

inline std::string ROCmFABackendToString(pluss::ROCmFABackend backend) {
  switch (backend) {
    case ROCmFABackend::Default:
      return "pluss::ROCmFABackend::Default";
    case ROCmFABackend::AOTriton:
      return "pluss::ROCmFABackend::AOTriton";
    case ROCmFABackend::Ck:
      return "pluss::ROCmFABackend::Ck";
    default:
      TORCH_CHECK(false, "Unknown ROCm flash attention backend")
  }
}

inline std::ostream& operator<<(
    std::ostream& stream,
    pluss::ROCmFABackend backend) {
  return stream << ROCmFABackendToString(backend);
}

} // namespace pluss
