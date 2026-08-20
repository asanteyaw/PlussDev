#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::where_ops {
  Series where(const Series& cond, const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series where(const Series& cond, const Series& a, const Series& b) {
    return pluss::codem::where_ops::where(cond, a, b);
  }
}
