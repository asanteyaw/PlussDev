#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::mps {

bool supports_cumsum_dim(ScalarType dt) noexcept;
bool supports_cumprod_dim(ScalarType dt) noexcept;

void cumsum_dim(const Series& in, std::int64_t dim, Series& out);
void cumprod_dim(const Series& in, std::int64_t dim, Series& out);

} // namespace pluss::codem::native::mps
