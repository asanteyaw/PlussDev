#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::gather_ops {
  Series gather(const Series& input, std::int64_t dim, const Series& index);
}

namespace pluss::codem::ops {
  inline Series gather(const Series& input, std::int64_t dim, const Series& index) {
    return pluss::codem::gather_ops::gather(input, dim, index);
  }
}
