#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::mps {

bool supports_cast(ScalarType src, ScalarType dst) noexcept;
void cast(const Series& in, Series& out);

} // namespace pluss::codem::native::mps
