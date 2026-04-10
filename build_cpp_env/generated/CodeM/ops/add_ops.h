#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::add_ops {
  Series add(const Series& a, const Series& b);
}

namespace pluss::codem::ops {
  inline Series add(const Series& a, const Series& b) {
    return pluss::codem::add_ops::add(a, b);
  }
}
