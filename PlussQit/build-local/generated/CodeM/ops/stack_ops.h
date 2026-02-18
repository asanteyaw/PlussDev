#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::stack_ops {
  Series stack(const std::vector<Series>& tensors, std::int64_t dim);
}

namespace pluss::codem::ops {
  inline Series stack(const std::vector<Series>& tensors, std::int64_t dim) {
    return pluss::codem::stack_ops::stack(tensors, dim);
  }
}
