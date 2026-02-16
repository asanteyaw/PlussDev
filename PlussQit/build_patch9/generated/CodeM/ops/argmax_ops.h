#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::argmax_ops {
  Series argmax(const Series& a, std::int64_t dim, bool keepdim);
}

namespace pluss::codem::ops {
  inline Series argmax(const Series& a, std::int64_t dim, bool keepdim) {
    return pluss::codem::argmax_ops::argmax(a, dim, keepdim);
  }
}
