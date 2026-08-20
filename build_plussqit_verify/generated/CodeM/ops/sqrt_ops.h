#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::sqrt_ops {
  Series sqrt(const Series& a);
}

namespace pluss::codem::ops {
  inline Series sqrt(const Series& a) {
    return pluss::codem::sqrt_ops::sqrt(a);
  }
}
