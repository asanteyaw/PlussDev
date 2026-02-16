#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::amax_ops {
  Series amax(const Series& a, std::int64_t dim, bool keepdim);
}

namespace pluss::codem::ops {
  inline Series amax(const Series& a, std::int64_t dim, bool keepdim) {
    return pluss::codem::amax_ops::amax(a, dim, keepdim);
  }
}
