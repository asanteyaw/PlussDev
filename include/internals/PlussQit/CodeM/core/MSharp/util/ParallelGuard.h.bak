#pragma once

#include <MSharp/macros/Macros.h>

namespace MSharp {

// RAII thread local guard that tracks whether code is being executed in
// `pluss::parallel_for` or `pluss::parallel_reduce` loop function.
class C10_API ParallelGuard {
 public:
  static bool is_enabled();

  ParallelGuard(bool state);
  ~ParallelGuard();

 private:
  bool previous_state_;
};

} // namespace MSharp
