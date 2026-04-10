#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::matmul_ops {
  Series matmul(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series matmul(const Series& a, const Series& b) {
    return pluss::codem::matmul_ops::matmul(a, b);
  }
}
