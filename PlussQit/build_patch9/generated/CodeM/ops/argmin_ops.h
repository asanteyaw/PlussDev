#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::argmin_ops {
  Series argmin(const Series& a, std::int64_t dim, bool keepdim);
}

namespace pluss::codem::ops {
  inline Series argmin(const Series& a, std::int64_t dim, bool keepdim) {
    return pluss::codem::argmin_ops::argmin(a, dim, keepdim);
  }
}
