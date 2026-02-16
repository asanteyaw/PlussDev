#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::sub_ops {
  Series sub(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series sub(const Series& a, const Series& b) {
    return pluss::codem::sub_ops::sub(a, b);
  }
}
