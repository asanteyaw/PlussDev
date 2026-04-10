#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::abs_ops {
  Series abs(const Series& a);
}

namespace pluss::codem::ops {
  inline Series abs(const Series& a) {
    return pluss::codem::abs_ops::abs(a);
  }
}
