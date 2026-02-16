#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::sum_ops {
  Series sum(const Series& a, std::int64_t dim, bool keepdim);
}

namespace pluss::codem::ops {
  inline Series sum(const Series& a, std::int64_t dim, bool keepdim) {
    return pluss::codem::sum_ops::sum(a, dim, keepdim);
  }
}
