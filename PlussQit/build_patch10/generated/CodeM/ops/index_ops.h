#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::index_ops {
  Series index(const Series& a, const std::vector<Series>& indices);
}

namespace pluss::codem::ops {
  inline Series index(const Series& a, const std::vector<Series>& indices) {
    return pluss::codem::index_ops::index(a, indices);
  }
}
