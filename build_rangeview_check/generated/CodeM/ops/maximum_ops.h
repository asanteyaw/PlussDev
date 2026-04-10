#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::maximum_ops {
  Series maximum(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series maximum(const Series& a, const Series& b) {
    return pluss::codem::maximum_ops::maximum(a, b);
  }
}
