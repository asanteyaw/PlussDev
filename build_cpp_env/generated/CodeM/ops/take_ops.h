#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::take_ops {
  Series take(const Series& a, const Series& index);
}

namespace pluss::codem::ops {
  inline Series take(const Series& a, const Series& index) {
    return pluss::codem::take_ops::take(a, index);
  }
}
