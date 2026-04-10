#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::cos_ops {
  Series cos(const Series& a);
}

namespace pluss::codem::ops {
  inline Series cos(const Series& a) {
    return pluss::codem::cos_ops::cos(a);
  }
}
