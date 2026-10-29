#pragma once

// Shared numerical glue for the domain tranches (biology, chemistry, ...):
// everything the model headers have in common lives here so they only state
// their equations.
//
//  * Coeff        - a parameter that is either a plain double or a Series that
//                   broadcasts against the state (per-species / per-replicate).
//  * validation   - finite / positive / non-negative / species-axis checks.
//  * integration  - batched RK4 over math::ode::fluxa for any rate function
//                   mapping a state Series [..., n] to its derivative.
//  * linear solve - batched solve via linearalg::inverse.
//
// States are Series with species on the LAST axis; every leading axis is an
// independent replicate / parameter-sweep lane and is never looped over.

#include "pluss/codem/codem.h"
#include "pluss/subsp/linearalg/linearalg.h"
#include "pluss/subsp/math/ode.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::common::dynamics {

using RateFn = std::function<Series(const Series &)>;

class Coeff final {
public:
  Coeff(double value) : scalar_(value) {}
  Coeff(Series value) : series_(std::move(value)), is_series_(true) {}

  // Materialise on the dtype / platform of `like` (no host round trip for
  // Series coefficients).
  [[nodiscard]] Series on(const Series &like) const {
    if (is_series_)
      return series_.to(like.scalar_type()).to(like.platform());
    return pluss::codem::make_scalar_like(like, scalar_);
  }

  [[nodiscard]] bool is_series() const noexcept { return is_series_; }
  [[nodiscard]] double scalar() const noexcept { return scalar_; }

private:
  double scalar_{0.0};
  Series series_ = pluss::series::undefined();
  bool is_series_{false};
};

inline void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::invalid_argument("natsci: " + message);
}

inline void require_float(const Series &value, const char *name) {
  require(value.dtype().is_floating(),
          std::string(name) + " must have a real floating dtype");
}

inline void require_finite(const Series &value, const char *name) {
  require_float(value, name);
  require(value.isfinite().all().item<bool>(),
          std::string(name) + " must be finite");
}

inline void require_nonnegative(const Series &value, const char *name) {
  require_finite(value, name);
  require(value.ge(0.0).all().item<bool>(),
          std::string(name) + " must be non-negative");
}

inline void require_positive(const Series &value, const char *name) {
  require_finite(value, name);
  require(value.gt(0.0).all().item<bool>(),
          std::string(name) + " must be strictly positive");
}

inline void require_positive(const Coeff &value, const Series &like,
                             const char *name) {
  if (!value.is_series()) {
    require(std::isfinite(value.scalar()) && value.scalar() > 0.0,
            std::string(name) + " must be finite and strictly positive");
    return;
  }
  require_positive(value.on(like), name);
}

inline void require_nonnegative(const Coeff &value, const Series &like,
                                const char *name) {
  if (!value.is_series()) {
    require(std::isfinite(value.scalar()) && value.scalar() >= 0.0,
            std::string(name) + " must be finite and non-negative");
    return;
  }
  require_nonnegative(value.on(like), name);
}

inline void require_species(const Series &state, std::int64_t species,
                            const char *name) {
  require(state.dim() >= 1 && state.size(-1) == species,
          std::string(name) + " must have " + std::to_string(species) +
              " species on its last axis");
}

inline void require_square(const Series &matrix, const char *name) {
  require(matrix.dim() >= 2 && matrix.size(-1) == matrix.size(-2),
          std::string(name) + " must be a (batch of) square matrices");
}

// Elementwise product of a vector state [..., n] with a matrix-vector product
// A n, where A is [n, n] or [..., n, n].
inline Series matvec(const Series &matrix, const Series &state) {
  // A shared 2-D matrix: fold every lane into one GEMM, [..., n] @ [n, m]
  // (a per-lane [1, n] @ [n, m] batch would be far slower).
  if (matrix.dim() == 2)
    return state.matmul(matrix.transpose(0, 1));
  return state.unsqueeze(-2).matmul(matrix.transpose(-1, -2)).squeeze(-2);
}

// Product over the last axis (zero-safe; no host loop).
inline Series prod_last(const Series &value) {
  return value.cumprod(-1, false).select(-1, -1);
}

// Solve A x = b for [..., n, n] x [..., n]; batched through one inverse.
inline Series solve(const Series &matrix, const Series &rhs) {
  require_square(matrix, "solve: matrix");
  return pluss::subsp::linearalg::inverse(matrix)
      .matmul(rhs.unsqueeze(-1))
      .squeeze(-1);
}

enum class Difference { Central, ForwardSecondOrder };

// Batched finite-difference Jacobian of a state map f: [..., n] -> [..., n],
// returned as [..., n(out), n(in)]. All n perturbations of all lanes go
// through f in ONE call (the perturbation axis sits just before the species
// axis), so f must treat leading axes as independent lanes. Central is
// O(h^2) and exact for quadratics; ForwardSecondOrder only evaluates at
// x, x+h e_j, x+2h e_j (use it on a domain boundary such as a disease-free
// state, where x - h e_j would leave the non-negative cone).
inline Series jacobian(const RateFn &f, const Series &x, double h = 1e-6,
                       Difference scheme = Difference::Central) {
  require(h > 0.0 && std::isfinite(h), "jacobian: step must be positive");
  const std::int64_t n = x.size(-1);
  const Config cfg =
      Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
  const Series e = pluss::series::eye(n, cfg).mul(h);
  const Series base = x.unsqueeze(-2);
  Series columns = pluss::series::undefined();
  if (scheme == Difference::Central) {
    columns = f(base.add(e)).sub(f(base.sub(e))).div(2.0 * h);
  } else {
    columns = f(base.add(e)).mul(4.0).sub(f(base.add(e.mul(2.0))))
                  .sub(f(base.add(e.mul(0.0))).mul(3.0)).div(2.0 * h);
  }
  return columns.transpose(-1, -2);
}

// Central-difference Jacobian with a per-component (and per-lane) step
// `step` [..., n]; use when components differ by orders of magnitude
// (concentrations vs temperature). Same layout as jacobian(): [..., n, n].
inline Series jacobian_scaled(const RateFn &f, const Series &x, const Series &step) {
  require(step.dim() == x.dim() && step.size(-1) == x.size(-1),
          "jacobian_scaled: step must match the state shape");
  require(step.gt(0.0).all().item<bool>(), "jacobian_scaled: steps must be positive");
  const std::int64_t n = x.size(-1);
  const Config cfg =
      Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
  const Series e = pluss::series::eye(n, cfg).mul(step.unsqueeze(-1)); // [..., n(in), n]
  const Series base = x.unsqueeze(-2);
  const Series columns =
      f(base.add(e)).sub(f(base.sub(e))).div(step.mul(2.0).unsqueeze(-1));
  return columns.transpose(-1, -2);
}

// Integrate dy/dt = rate(y) from t0 to t1 with fixed-step RK4. The state may
// carry any leading batch axes; all lanes advance in the same Series ops.
inline Series simulate(const RateFn &rate, const Series &y0, double t0,
                       double t1, std::int64_t steps = 200) {
  require(static_cast<bool>(rate), "simulate: rate callback is required");
  require(steps > 0, "simulate: steps must be positive");
  require(std::isfinite(t0) && std::isfinite(t1),
          "simulate: times must be finite");
  require_finite(y0, "simulate: initial state");
  pluss::subsp::math::ode::OdeOptions options;
  options.steps = steps;
  return pluss::subsp::math::ode::fluxa(
             [&rate](double, const Series &y, const Series &) {
               return rate(y);
             },
             y0, t0, t1, pluss::series::undefined(), options)
      .y_final;
}

struct Path final {
  Series times = pluss::series::undefined(); // [samples + 1]
  Series states = pluss::series::undefined(); // [samples + 1, ..., n]
};

// Saved trajectory: `samples` equal intervals, each integrated with
// `steps_per_sample` RK4 steps. Sequential in time by nature; batched across
// lanes.
inline Path trajectory(const RateFn &rate, const Series &y0, double t0,
                       double t1, std::int64_t samples,
                       std::int64_t steps_per_sample = 50) {
  require(samples > 0, "trajectory: samples must be positive");
  std::vector<Series> frames;
  frames.reserve(static_cast<std::size_t>(samples) + 1);
  frames.push_back(y0.unsqueeze(0));
  Series y = y0;
  const double dt = (t1 - t0) / static_cast<double>(samples);
  for (std::int64_t k = 0; k < samples; ++k) {
    y = simulate(rate, y, t0 + dt * static_cast<double>(k),
                 t0 + dt * static_cast<double>(k + 1), steps_per_sample);
    frames.push_back(y.unsqueeze(0));
  }
  Series stacked = frames.front();
  if (frames.size() > 1)
    stacked = stacked.unite(
        std::vector<Series>(frames.begin() + 1, frames.end()), 0);
  Series times = pluss::series::linspace(t0, t1, samples + 1, y0.scalar_type())
                     .to(y0.platform());
  return {std::move(times), std::move(stacked)};
}

} // namespace pluss::subsp::natsci::common::dynamics
