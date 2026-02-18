#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::ge_ops {
  Series ge(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series ge(const Series& a, const Series& b) {
    return pluss::codem::ge_ops::ge(a, b);
  }
}
