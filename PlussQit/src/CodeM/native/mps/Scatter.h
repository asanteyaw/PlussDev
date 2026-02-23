#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::mps {

enum class ScatterReduceOp { Sum, AMax, AMin };

bool supports_scatter(ScalarType out_dt, ScalarType index_dt, ScalarType src_dt) noexcept;
void scatter_set_(Series& out, std::int64_t dim, const Series& index, const Series& src);

bool supports_scatter_reduce(ScalarType out_dt, ScalarType index_dt, ScalarType src_dt) noexcept;
void scatter_reduce_(Series& out, std::int64_t dim, const Series& index, const Series& src, ScatterReduceOp op);

} // namespace pluss::codem::native::mps
