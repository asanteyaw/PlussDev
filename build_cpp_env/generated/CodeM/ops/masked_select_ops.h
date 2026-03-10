#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::masked_select_ops {
  Series masked_select(const Series& a, const Series& mask);
}

namespace pluss::codem::ops {
  inline Series masked_select(const Series& a, const Series& mask) {
    return pluss::codem::masked_select_ops::masked_select(a, mask);
  }
}
