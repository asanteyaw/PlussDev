#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

#include <string_view>

namespace pluss::codem::native::cuda {

bool native_scatter_gather_enabled() noexcept;
bool native_scatter_gather_strict() noexcept;

bool supports_scatter_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept;
bool supports_scatter_add_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept;
bool supports_scatter_reduce_baseline(
    ScalarType self_dtype,
    ScalarType index_dtype,
    ScalarType src_dtype,
    std::string_view reduce) noexcept;
bool supports_gather_baseline(ScalarType input_dtype, ScalarType index_dtype) noexcept;

bool supports_scatter(const Series& self, std::int64_t dim, const Series& index, const Series& src) noexcept;
bool supports_scatter_add(const Series& self, std::int64_t dim, const Series& index, const Series& src) noexcept;
bool supports_scatter_reduce(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& src,
    std::string_view reduce) noexcept;
bool supports_gather(const Series& input, std::int64_t dim, const Series& index) noexcept;

} // namespace pluss::codem::native::cuda
