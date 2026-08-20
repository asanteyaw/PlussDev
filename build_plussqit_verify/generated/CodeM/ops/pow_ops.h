#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::pow_ops {
  Series pow(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series pow(const Series& a, const Series& b) {
    return pluss::codem::pow_ops::pow(a, b);
  }
}
