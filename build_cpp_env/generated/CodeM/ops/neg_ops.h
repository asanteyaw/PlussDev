#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::neg_ops {
  Series neg(const Series& a);
}

namespace pluss::codem::ops {
  inline Series neg(const Series& a) {
    return pluss::codem::neg_ops::neg(a);
  }
}
