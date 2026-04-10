#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::nonzero_ops {
  Series nonzero(const Series& mask);
}

namespace pluss::codem::ops {
  inline Series nonzero(const Series& mask) {
    return pluss::codem::nonzero_ops::nonzero(mask);
  }
}
