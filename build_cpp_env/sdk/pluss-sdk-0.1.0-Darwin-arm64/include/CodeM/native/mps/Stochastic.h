#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

#include <cstdint>

namespace pluss::codem::native::mps {

bool supports_uniform_stochastic(ScalarType dtype) noexcept;
bool supports_normal_stochastic(ScalarType dtype) noexcept;

Series uniform_stochastic(pluss::msharp::RangeView::Int sizes,
                          ScalarType dtype,
                          double from,
                          double to,
                          std::uint64_t seed);

Series normal_stochastic(pluss::msharp::RangeView::Int sizes,
                         ScalarType dtype,
                         double mean,
                         double std,
                         std::uint64_t seed);

} // namespace pluss::codem::native::mps
