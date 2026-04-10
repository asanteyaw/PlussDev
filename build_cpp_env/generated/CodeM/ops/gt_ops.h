#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::gt_ops {
  Series gt(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series gt(const Series& a, const Series& b) {
    return pluss::codem::gt_ops::gt(a, b);
  }
}
