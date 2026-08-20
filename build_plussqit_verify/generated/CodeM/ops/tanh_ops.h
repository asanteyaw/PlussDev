#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::tanh_ops {
  Series tanh(const Series& a);
}

namespace pluss::codem::ops {
  inline Series tanh(const Series& a) {
    return pluss::codem::tanh_ops::tanh(a);
  }
}
