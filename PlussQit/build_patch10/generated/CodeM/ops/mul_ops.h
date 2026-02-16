#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::mul_ops {
  Series mul(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series mul(const Series& a, const Series& b) {
    return pluss::codem::mul_ops::mul(a, b);
  }
}
