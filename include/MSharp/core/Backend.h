#pragma once

// Minimal stub of Backend enum to satisfy Layout mapping
#include <cstdint>

namespace MSharp {
enum class Backend : int8_t {
  SparseCPU,
  SparseCUDA,
  SparseHIP,
  SparseVE,
  SparseXPU,
  MkldnnCPU,
  SparseCsrCPU,
  SparseCsrCUDA,
  SparseCsrHIP,
  SparseCsrVE,
  SparseCsrXPU,
  SparsePrivateUse1,
  Undefined,
};

} // namespace MSharp

namespace MSharp {
using Backend = MSharp::Backend;
} // namespace MSharp
