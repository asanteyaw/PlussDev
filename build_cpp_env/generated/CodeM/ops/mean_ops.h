#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::mean_ops {
  Series mean(const Series& a, std::int64_t dim, bool keepdim);
}

namespace pluss::codem::ops {
  inline Series mean(const Series& a, std::int64_t dim, bool keepdim) {
    return pluss::codem::mean_ops::mean(a, dim, keepdim);
  }
}
