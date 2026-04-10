#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::lt_ops {
  Series lt(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series lt(const Series& a, const Series& b) {
    return pluss::codem::lt_ops::lt(a, b);
  }
}
