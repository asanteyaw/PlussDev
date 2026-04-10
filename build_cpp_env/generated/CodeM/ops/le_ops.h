#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::le_ops {
  Series le(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series le(const Series& a, const Series& b) {
    return pluss::codem::le_ops::le(a, b);
  }
}
