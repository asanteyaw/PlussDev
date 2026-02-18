#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::minimum_ops {
  Series minimum(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series minimum(const Series& a, const Series& b) {
    return pluss::codem::minimum_ops::minimum(a, b);
  }
}
