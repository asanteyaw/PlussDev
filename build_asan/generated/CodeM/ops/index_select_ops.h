#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::index_select_ops {
  Series index_select(const Series& a, std::int64_t dim, const Series& index);
}

namespace pluss::codem::ops {
  inline Series index_select(const Series& a, std::int64_t dim, const Series& index) {
    return pluss::codem::index_select_ops::index_select(a, dim, index);
  }
}
