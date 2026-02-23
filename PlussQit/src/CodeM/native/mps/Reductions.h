#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::mps {

bool supports_sum_dim(ScalarType dt) noexcept;
void sum_dim(const Series& a, std::int64_t dim, Series& out);
bool supports_prod_dim(ScalarType dt) noexcept;
void prod_dim(const Series& a, std::int64_t dim, Series& out);

bool supports_amax_amin_dim(ScalarType dt) noexcept;
void amax_dim(const Series& a, std::int64_t dim, Series& out);
void amin_dim(const Series& a, std::int64_t dim, Series& out);

bool supports_all_any_dim(ScalarType in_dt) noexcept;
void all_dim(const Series& a, std::int64_t dim, Series& out);
void any_dim(const Series& a, std::int64_t dim, Series& out);

bool supports_argmax_argmin_dim(ScalarType dt) noexcept;
void argmax_dim(const Series& a, std::int64_t dim, Series& out);
void argmin_dim(const Series& a, std::int64_t dim, Series& out);

} // namespace pluss::codem::native::mps
