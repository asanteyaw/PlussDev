#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::eq_ops {
  Series eq(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series eq(const Series& a, const Series& b) {
    return pluss::codem::eq_ops::eq(a, b);
  }
}
