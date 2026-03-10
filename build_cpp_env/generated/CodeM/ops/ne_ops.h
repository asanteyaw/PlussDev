#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::ne_ops {
  Series ne(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series ne(const Series& a, const Series& b) {
    return pluss::codem::ne_ops::ne(a, b);
  }
}
