#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"
#include "MSharp/RangeView.h"

#include <cstdint>

namespace pluss::codem::native::cuda {

bool native_compose_enabled() noexcept;
bool native_compose_strict() noexcept;

bool supports_compose_baseline(ScalarType dtype) noexcept;

bool supports_unite_compose(
    const Series& self,
    pluss::msharp::RangeView::Of<Series> others,
    std::int64_t dim) noexcept;

bool supports_pile_compose(
    const Series& self,
    pluss::msharp::RangeView::Of<Series> others,
    std::int64_t dim) noexcept;

} // namespace pluss::codem::native::cuda
