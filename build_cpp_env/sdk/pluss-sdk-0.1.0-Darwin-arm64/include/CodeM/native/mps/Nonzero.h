#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::mps {

bool supports_nonzero(ScalarType mask_dt) noexcept;
Series nonzero(const Series& mask);

} // namespace pluss::codem::native::mps
