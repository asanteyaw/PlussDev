#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::scatter_ops {
  Series scatter(const Series& input, std::int64_t dim, const Series& index, const Series& src);
}

namespace pluss::codem::ops {
  inline Series scatter(const Series& input, std::int64_t dim, const Series& index, const Series& src) {
    return pluss::codem::scatter_ops::scatter(input, dim, index, src);
  }
}
