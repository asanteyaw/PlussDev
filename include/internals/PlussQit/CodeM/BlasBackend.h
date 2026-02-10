#pragma once

#include <MSharp/util/Exception.h>

#include <ostream>
#include <string>

namespace pluss {

enum class BlasBackend : int8_t { Default, Cublas, Cublaslt, Ck };

inline std::string BlasBackendToString(pluss::BlasBackend backend) {
  switch (backend) {
    case BlasBackend::Default:
      return "pluss::BlasBackend::Default";
    case BlasBackend::Cublas:
      return "pluss::BlasBackend::Cublas";
    case BlasBackend::Cublaslt:
      return "pluss::BlasBackend::Cublaslt";
    case BlasBackend::Ck:
      return "pluss::BlasBackend::Ck";
    default:
      TORCH_CHECK(false, "Unknown blas backend");
  }
}

inline std::ostream& operator<<(std::ostream& stream, pluss::BlasBackend backend) {
  return stream << BlasBackendToString(backend);
}

} // namespace pluss
