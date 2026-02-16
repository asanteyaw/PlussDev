#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::amin_ops {
  Series amin(const Series& a, std::int64_t dim, bool keepdim);
}

namespace pluss::codem::ops {
  inline Series amin(const Series& a, std::int64_t dim, bool keepdim) {
    return pluss::codem::amin_ops::amin(a, dim, keepdim);
  }
}
