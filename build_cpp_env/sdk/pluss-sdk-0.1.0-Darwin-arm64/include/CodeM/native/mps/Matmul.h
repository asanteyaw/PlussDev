#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::mps {

bool supports_matmul(const Series& a, const Series& b) noexcept;
Series matmul(const Series& a, const Series& b);

} // namespace pluss::codem::native::mps
