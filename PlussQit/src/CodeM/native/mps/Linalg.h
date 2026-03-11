#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::mps {

struct LuDetStats {
  float sign{1.0f};
  float logabs{0.0f};
  bool singular{false};
};

bool supports_lu_solver(const Series& a) noexcept;
bool supports_lu_solve(const Series& a, const Series& b) noexcept;
Series lu_solve(const Series& a, const Series& b);
Series lu_inverse(const Series& a);
LuDetStats lu_det_stats(const Series& a);

bool supports_cholesky_factor(const Series& a) noexcept;
Series cholesky_factor(const Series& a, bool upper);
bool supports_cholesky_solve(const Series& factor, const Series& b) noexcept;
Series cholesky_solve(const Series& factor, const Series& b, bool upper);

} // namespace pluss::codem::native::mps
