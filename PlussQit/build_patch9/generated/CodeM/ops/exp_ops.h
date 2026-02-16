#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::exp_ops {
  Series exp(const Series& a);
}

namespace pluss::codem::ops {
  inline Series exp(const Series& a) {
    return pluss::codem::exp_ops::exp(a);
  }
}
