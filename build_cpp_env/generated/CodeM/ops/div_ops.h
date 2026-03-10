#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::div_ops {
  Series div(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series div(const Series& a, const Series& b) {
    return pluss::codem::div_ops::div(a, b);
  }
}
