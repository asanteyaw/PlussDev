#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::slice_ops {
  Series slice(const Series& a, std::int64_t dim, std::int64_t start, std::int64_t end, std::int64_t step);
}

namespace pluss::codem::ops {
  inline Series slice(const Series& a, std::int64_t dim, std::int64_t start, std::int64_t end, std::int64_t step) {
    return pluss::codem::slice_ops::slice(a, dim, start, end, step);
  }
}
