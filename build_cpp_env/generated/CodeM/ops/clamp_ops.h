#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::clamp_ops {
  Series clamp(const Series& a, double lo, double hi);
}

namespace pluss::codem::ops {
  inline Series clamp(const Series& a, double lo, double hi) {
    return pluss::codem::clamp_ops::clamp(a, lo, hi);
  }
}
