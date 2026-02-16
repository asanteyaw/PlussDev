#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::sign_ops {
  Series sign(const Series& a);
}

namespace pluss::codem::ops {
  inline Series sign(const Series& a) {
    return pluss::codem::sign_ops::sign(a);
  }
}
