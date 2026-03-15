#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::log_ops {
  Series log(const Series& a);
}

namespace pluss::codem::ops {
  inline Series log(const Series& a) {
    return pluss::codem::log_ops::log(a);
  }
}
