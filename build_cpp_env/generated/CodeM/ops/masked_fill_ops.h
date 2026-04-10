#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::masked_fill_ops {
  Series masked_fill(const Series& a, const Series& mask, double value);
}

namespace pluss::codem::ops {
  inline Series masked_fill(const Series& a, const Series& mask, double value) {
    return pluss::codem::masked_fill_ops::masked_fill(a, mask, value);
  }
}
