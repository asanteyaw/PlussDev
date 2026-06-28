#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::mps {

bool supports_gather(ScalarType input_dt, ScalarType index_dt) noexcept;
void gather(const Series& input, std::int64_t dim, const Series& index, Series& out);

} // namespace pluss::codem::native::mps
