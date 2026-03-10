#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::scatter_add_ops {
  Series scatter_add(const Series& input, std::int64_t dim, const Series& index, const Series& src);
}

namespace pluss::codem::ops {
  inline Series scatter_add(const Series& input, std::int64_t dim, const Series& index, const Series& src) {
    return pluss::codem::scatter_add_ops::scatter_add(input, dim, index, src);
  }
}
