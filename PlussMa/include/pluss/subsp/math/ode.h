#pragma once

// pluss::subsp::math::ode -- Series-native ODE/IVP integration, with
// adjoint-sensitivity gradients.
//
// Naming: **Fluxa** (Latin, "a flowing" -- the literal concept an ODE
// describes: dy/dt is the FLOW/flux of the state y through time), the
// forward/adjoint solver pair, matching the Latin-adjective family this
// program's numerical modules already use (Septima/Optima/Minimax under
// pluss::optim; Integra/Quadra/Aleatoria/Analytica under pluss::integ).
//
// Why this is its OWN module, not folded into pluss::integ: solving an
// initial-value problem (integrating a differential equation forward in
// time) is a genuinely different mathematical object from integrating a
// fixed function over a domain, even though both use the word
// "integration" colloquially. Everywhere else in pluss::integ,
// differentiability is free (composed entirely from already-
// differentiable Series primitives, no custom backward pass needed
// anywhere). An ODE solve is the one place that stops being true: naively
// autodiff-ing through an unrolled RK loop keeps the ENTIRE trajectory's
// computation graph in memory (`O(steps)` tape nodes), which is exactly
// the problem the adjoint method (Chen et al. 2018, "Neural Ordinary
// Differential Equations"; the same technique torchdiffeq/diffrax are
// built around) exists to solve -- a genuinely separate backward-pass
// design, not something inherited for free.
//
// The adjoint method itself, though, is built by REUSING Pluss's existing
// reverse-mode engine rather than hand-deriving Jacobians: at each
// backward step, `f`'s own already-differentiable Series body is used to
// compute that one step's local vector-Jacobian product via an ordinary
// `.reverse()` call on a freshly-tracked leaf -- exactly the same
// "compose from already-differentiable primitives, inherit the property"
// philosophy used throughout pluss::integ, just applied per-step instead
// of once over the whole computation.
//
// Scope/tradeoffs, stated plainly:
//  - `fluxa()` provides fixed-step RK4 and adaptive Dormand-Prince 5(4).
//    RK45 controls a weighted RMS local error and lands exactly on the
//    requested endpoint. The checkpointed discrete adjoint below remains an
//    RK4 contract because differentiating accepted/rejected adaptive step
//    decisions requires a separate policy rather than pretending the two
//    discretizations are interchangeable.
//  - `fluxa_adjoint()`'s backward pass is a CHECKPOINTED discrete adjoint:
//    it stores the forward trajectory's y values at every step boundary
//    (O(steps) memory for the trajectory itself) and computes per-step
//    vector-Jacobian products going backward, rather than keeping ONE
//    giant autodiff graph spanning the whole trajectory in Series' own
//    tape at once. This is NOT the theoretically-optimal O(1)-memory
//    continuous-adjoint-ODE method (which would re-integrate y backward
//    via -h steps instead of storing it, at the cost of a real,
//    literature-acknowledged risk of the re-integrated trajectory
//    drifting from the true forward one for stiff/chaotic systems) --
//    that refinement is documented, real future work, not attempted here
//    given the correctness risk of an unverified reintegration-drift
//    assumption. What IS built is still a genuine, separate adjoint-style
//    backward pass (per-step reverse accumulation, not one unrolled
//    graph), verified independently against direct autodiff through the
//    identical RK4 discretization AND against a hand-derived analytic
//    closed form (see the test file).
//  - Only a flat 1-D `y`/`theta` state is supported (matching
//    IntegrandFn's own "flatten your own problem into one Series" +
//    Options/Result convention throughout this file). A caller with a
//    structured state (multiple named variables) flattens/unflattens it
//    themselves, exactly like Septima's own multi-block ergonomics do.

#include "pluss/subsp/math/diff.h"
#include "pluss/subsp/math/krylov.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace pluss::subsp::math::ode {

// dy/dt = f(t, y, theta). `t` is a plain double (ODE time is not part of
// the Series/autodiff system -- only y and theta are). `theta` may be an
// undefined Series if the right-hand side has no parameters the caller
// wants gradients against (everything it needs already captured directly
// in the closure).
using OdeFn = std::function<Series(double, const Series &, const Series &)>;
using OdeEventFn =
    std::function<Series(double, const Series &, const Series &)>;
using LinearOdeFn = std::function<Series(double, const Series &)>;

enum class OdeMethod : std::uint8_t {
  RK4 = 0,
  RK45 = 1,
};

struct OdeOptions {
  OdeMethod method{OdeMethod::RK4};
  std::int64_t steps{100};
  double relative_tolerance{1e-6};
  double absolute_tolerance{1e-9};
  double initial_step{0.0};
  double minimum_step{0.0};
  double maximum_step{std::numeric_limits<double>::infinity()};
  std::int64_t max_steps{100000};
};

struct OdeResult {
  Series y_final = pluss::series::undefined();
  std::int64_t steps{0};
  std::int64_t function_evaluations{0};
  std::int64_t rejected_steps{0};
  bool converged{false};
};

enum class OdeEventDirection : std::int8_t {
  Any = 0,
  Rising = 1,
  Falling = -1,
};

struct OdeEventOptions {
  OdeEventDirection direction{OdeEventDirection::Any};
  double time_tolerance{1e-10};
  double value_tolerance{1e-10};
  std::int64_t max_refinements{48};
};

struct OdeEventResult : OdeResult {
  bool triggered{false};
  double t_final{0.0};
  Series event_value = pluss::series::undefined();
  std::int64_t event_evaluations{0};
};

struct OdeAdjointResult {
  Series y_final = pluss::series::undefined();
  Series grad_y0 = pluss::series::undefined();
  Series grad_theta =
      pluss::series::undefined(); // undefined if `theta` was undefined
  std::int64_t forward_steps{0};
  std::int64_t backward_steps{0};
};

struct LinearOdeOptions {
  std::int64_t steps{100};
  double relative_tolerance{1e-6};
  double absolute_tolerance{1e-9};
  double initial_step{0.0};
  double minimum_step{0.0};
  double maximum_step{std::numeric_limits<double>::infinity()};
  std::int64_t max_steps{100000};
  std::vector<std::int64_t> solve_dims{-1};
  krylov::GmresOptions linear{};
};

namespace detail {

inline void require_defined_(const Series &x, const char *op) {
  if (!x.defined()) {
    ::pluss::throw_error(std::string("subsp::math::ode::") + op +
                         ": expected defined Series");
  }
}

// One classical RK4 step (Series-native, differentiable for free through
// f's own already-differentiable body -- no custom backward rule here
// either, the same "compose, inherit" property everywhere else in this
// file has).
inline Series rk4_step_(const OdeFn &f, double t, double h, const Series &y,
                        const Series &theta) {
  const Series k1 = f(t, y, theta);
  const Series k2 = f(t + h * 0.5, y.add(k1.mul(h * 0.5)), theta);
  const Series k3 = f(t + h * 0.5, y.add(k2.mul(h * 0.5)), theta);
  const Series k4 = f(t + h, y.add(k3.mul(h)), theta);
  return y.add(k1.add(k2.mul(2.0)).add(k3.mul(2.0)).add(k4).mul(h / 6.0));
}

struct Rk45Step final {
  Series value;
  Series error;
};

inline Rk45Step rk45_step_(const OdeFn &f, double t, double h, const Series &y,
                           const Series &theta) {
  const Series k1 = f(t, y, theta);
  const Series k2 =
      f(t + h * (1.0 / 5.0), y.add(k1.mul(h * (1.0 / 5.0))), theta);
  const Series k3 =
      f(t + h * (3.0 / 10.0),
        y.add(k1.mul(h * (3.0 / 40.0))).add(k2.mul(h * (9.0 / 40.0))), theta);
  const Series k4 = f(t + h * (4.0 / 5.0),
                      y.add(k1.mul(h * (44.0 / 45.0)))
                          .add(k2.mul(h * (-56.0 / 15.0)))
                          .add(k3.mul(h * (32.0 / 9.0))),
                      theta);
  const Series k5 = f(t + h * (8.0 / 9.0),
                      y.add(k1.mul(h * (19372.0 / 6561.0)))
                          .add(k2.mul(h * (-25360.0 / 2187.0)))
                          .add(k3.mul(h * (64448.0 / 6561.0)))
                          .add(k4.mul(h * (-212.0 / 729.0))),
                      theta);
  const Series k6 = f(t + h,
                      y.add(k1.mul(h * (9017.0 / 3168.0)))
                          .add(k2.mul(h * (-355.0 / 33.0)))
                          .add(k3.mul(h * (46732.0 / 5247.0)))
                          .add(k4.mul(h * (49.0 / 176.0)))
                          .add(k5.mul(h * (-5103.0 / 18656.0))),
                      theta);
  const Series high = y.add(k1.mul(h * (35.0 / 384.0)))
                          .add(k3.mul(h * (500.0 / 1113.0)))
                          .add(k4.mul(h * (125.0 / 192.0)))
                          .add(k5.mul(h * (-2187.0 / 6784.0)))
                          .add(k6.mul(h * (11.0 / 84.0)));
  const Series k7 = f(t + h, high, theta);
  const Series low = y.add(k1.mul(h * (5179.0 / 57600.0)))
                         .add(k3.mul(h * (7571.0 / 16695.0)))
                         .add(k4.mul(h * (393.0 / 640.0)))
                         .add(k5.mul(h * (-92097.0 / 339200.0)))
                         .add(k6.mul(h * (187.0 / 2100.0)))
                         .add(k7.mul(h * (1.0 / 40.0)));
  return {high, high.sub(low)};
}

inline double weighted_error_(const Series &error, const Series &left,
                              const Series &right, double rtol, double atol) {
  const Series scale = left.abs().maximum(right.abs()).mul(rtol).add(atol);
  return error.abs().div(scale).square().mean().sqrt().item<double>();
}

inline void validate_adaptive_options_(const OdeOptions &options) {
  if (!(options.relative_tolerance > 0.0) ||
      !std::isfinite(options.relative_tolerance) ||
      !(options.absolute_tolerance > 0.0) ||
      !std::isfinite(options.absolute_tolerance) ||
      !(options.initial_step >= 0.0) || !std::isfinite(options.initial_step) ||
      !(options.minimum_step >= 0.0) || !std::isfinite(options.minimum_step) ||
      !(options.maximum_step > 0.0) || std::isnan(options.maximum_step) ||
      options.max_steps <= 0 || options.minimum_step > options.maximum_step) {
    ::pluss::throw_error(
        "subsp::math::ode::fluxa: invalid adaptive-step options");
  }
}

inline void validate_adaptive_options_(const LinearOdeOptions &options) {
  if (options.steps <= 0 || !(options.relative_tolerance > 0.0) ||
      !std::isfinite(options.relative_tolerance) ||
      !(options.absolute_tolerance > 0.0) ||
      !std::isfinite(options.absolute_tolerance) ||
      !(options.initial_step >= 0.0) || !std::isfinite(options.initial_step) ||
      !(options.minimum_step >= 0.0) || !std::isfinite(options.minimum_step) ||
      !(options.maximum_step > 0.0) || std::isnan(options.maximum_step) ||
      options.max_steps <= 0 || options.minimum_step > options.maximum_step ||
      options.solve_dims.empty())
    ::pluss::throw_error(
        "subsp::math::ode::fluxa_linear: invalid adaptive-step options");
}

struct MidpointStep final {
  Series value;
  std::int64_t evaluations{};
  bool converged{};
};

inline MidpointStep midpoint_step_(const LinearOdeFn &action, double t,
                                   double h, const Series &y,
                                   const LinearOdeOptions &options) {
  const double midpoint = t + h * 0.5;
  const Series rhs = y.add(action(midpoint, y).mul(h * 0.5));
  auto system = [&](const Series &trial) {
    return trial.sub(action(midpoint, trial).mul(h * 0.5));
  };
  const auto solved =
      krylov::gmres(system, rhs, options.solve_dims, options.linear, y);
  return {solved.solution, solved.operator_evaluations + 1,
          solved.converged.all().item<bool>()};
}

inline double event_scalar_(const OdeEventFn &event, double t,
                            const Series &state, const Series &theta,
                            Series *raw = nullptr) {
  Series value = event(t, state, theta);
  require_defined_(value, "fluxa_until event");
  if (value.numel() != 1 || !value.dtype().is_floating()) {
    ::pluss::throw_error("subsp::math::ode::fluxa_until: event must return one "
                         "real floating value");
  }
  const double scalar =
      value.to(pluss::pCPU, ScalarType::Float64).item<double>();
  if (!std::isfinite(scalar)) {
    ::pluss::throw_error(
        "subsp::math::ode::fluxa_until: event returned a non-finite value");
  }
  if (raw)
    *raw = std::move(value);
  return scalar;
}

inline bool crosses_(double left, double right, OdeEventDirection direction) {
  switch (direction) {
  case OdeEventDirection::Rising:
    return left < 0.0 && right >= 0.0;
  case OdeEventDirection::Falling:
    return left > 0.0 && right <= 0.0;
  case OdeEventDirection::Any:
    return (left < 0.0 && right >= 0.0) || (left > 0.0 && right <= 0.0);
  }
  return false;
}

} // namespace detail

inline OdeResult fluxa(const OdeFn &f, const Series &y0, double t0, double t1,
                       const Series &theta = pluss::series::undefined(),
                       const OdeOptions &options = {}) {
  if (!f) {
    ::pluss::throw_error(
        "subsp::math::ode::fluxa: right-hand-side callback is required");
  }
  detail::require_defined_(y0, "fluxa");
  if (options.steps <= 0) {
    ::pluss::throw_error("subsp::math::ode::fluxa: steps must be > 0");
  }

  if (options.method == OdeMethod::RK45) {
    detail::validate_adaptive_options_(options);
    OdeResult result;
    result.y_final = y0;
    if (t0 == t1) {
      result.converged = true;
      return result;
    }
    const double direction = t1 > t0 ? 1.0 : -1.0;
    const double span = std::abs(t1 - t0);
    double magnitude = options.initial_step > 0.0
                           ? options.initial_step
                           : span / static_cast<double>(options.steps);
    magnitude = std::min(magnitude, options.maximum_step);
    if (options.minimum_step > 0.0)
      magnitude = std::max(magnitude, options.minimum_step);
    Series y = y0;
    double t = t0;
    for (std::int64_t attempt = 0; attempt < options.max_steps; ++attempt) {
      const double remaining = std::abs(t1 - t);
      if (remaining <= std::numeric_limits<double>::epsilon() *
                           std::max({1.0, std::abs(t), std::abs(t1)})) {
        result.y_final = y;
        result.converged = true;
        return result;
      }
      magnitude = std::min(magnitude, remaining);
      if (!(magnitude > 0.0) || t + direction * magnitude == t)
        ::pluss::throw_error(
            "subsp::math::ode::fluxa: adaptive step underflow");
      const double h = direction * magnitude;
      const auto candidate = detail::rk45_step_(f, t, h, y, theta);
      result.function_evaluations += 7;
      const double error = detail::weighted_error_(
          candidate.error, y, candidate.value, options.relative_tolerance,
          options.absolute_tolerance);
      if (!std::isfinite(error))
        ::pluss::throw_error(
            "subsp::math::ode::fluxa: non-finite adaptive error estimate");
      const bool accept = error <= 1.0;
      if (accept) {
        y = candidate.value;
        t += h;
        ++result.steps;
      } else {
        ++result.rejected_steps;
      }
      const double factor = error == 0.0
                                ? 10.0
                                : std::clamp(0.9 * std::pow(error, -0.2), 0.2,
                                             accept ? 10.0 : 1.0);
      magnitude = std::min(options.maximum_step, magnitude * factor);
      if (options.minimum_step > 0.0 && magnitude < options.minimum_step) {
        if (!accept)
          ::pluss::throw_error(
              "subsp::math::ode::fluxa: required step is below minimum_step");
        magnitude = options.minimum_step;
      }
    }
    ::pluss::throw_error(
        "subsp::math::ode::fluxa: adaptive step budget exhausted");
  }

  const double h = (t1 - t0) / static_cast<double>(options.steps);
  OdeResult result;
  Series y = y0;
  double t = t0;
  for (std::int64_t i = 0; i < options.steps; ++i) {
    y = detail::rk4_step_(f, t, h, y, theta);
    t += h;
    result.function_evaluations += 4;
  }
  result.y_final = y;
  result.steps = options.steps;
  result.converged = true;
  return result;
}

// Adaptive implicit midpoint with step doubling for a linear, possibly stiff
// system y' = A(t)y. The callback applies A(t) without requiring a dense
// matrix; each implicit solve delegates to the shared batched GMRES engine.
inline OdeResult fluxa_linear(const LinearOdeFn &action, const Series &y0,
                              double t0, double t1,
                              const LinearOdeOptions &options = {}) {
  if (!action)
    ::pluss::throw_error(
        "subsp::math::ode::fluxa_linear: linear action is required");
  detail::require_defined_(y0, "fluxa_linear");
  detail::validate_adaptive_options_(options);

  OdeResult result;
  result.y_final = y0;
  if (t0 == t1) {
    result.converged = true;
    return result;
  }
  const double direction = t1 > t0 ? 1.0 : -1.0;
  const double span = std::abs(t1 - t0);
  double magnitude = options.initial_step > 0.0
                         ? options.initial_step
                         : span / static_cast<double>(options.steps);
  magnitude = std::min(magnitude, options.maximum_step);
  if (options.minimum_step > 0.0)
    magnitude = std::max(magnitude, options.minimum_step);

  Series y = y0;
  double t = t0;
  for (std::int64_t attempt = 0; attempt < options.max_steps; ++attempt) {
    const double remaining = std::abs(t1 - t);
    if (remaining <= std::numeric_limits<double>::epsilon() *
                         std::max({1.0, std::abs(t), std::abs(t1)})) {
      result.y_final = y;
      result.converged = true;
      return result;
    }
    magnitude = std::min(magnitude, remaining);
    if (!(magnitude > 0.0) || t + direction * magnitude == t)
      ::pluss::throw_error(
          "subsp::math::ode::fluxa_linear: adaptive step underflow");
    const double h = direction * magnitude;

    const auto full = detail::midpoint_step_(action, t, h, y, options);
    result.function_evaluations += full.evaluations;
    if (!full.converged) {
      ++result.rejected_steps;
      magnitude *= 0.5;
      if (options.minimum_step > 0.0 && magnitude < options.minimum_step)
        ::pluss::throw_error(
            "subsp::math::ode::fluxa_linear: linear solve failed at "
            "minimum_step");
      continue;
    }
    const auto half_left =
        detail::midpoint_step_(action, t, h * 0.5, y, options);
    result.function_evaluations += half_left.evaluations;
    if (!half_left.converged) {
      ++result.rejected_steps;
      magnitude *= 0.5;
      continue;
    }
    const auto half_right = detail::midpoint_step_(action, t + h * 0.5, h * 0.5,
                                                   half_left.value, options);
    result.function_evaluations += half_right.evaluations;
    if (!half_right.converged) {
      ++result.rejected_steps;
      magnitude *= 0.5;
      continue;
    }

    const Series error = half_right.value.sub(full.value).div(3.0);
    const double scaled_error = detail::weighted_error_(
        error, y, half_right.value, options.relative_tolerance,
        options.absolute_tolerance);
    if (!std::isfinite(scaled_error))
      ::pluss::throw_error(
          "subsp::math::ode::fluxa_linear: non-finite error estimate");
    const bool accept = scaled_error <= 1.0;
    if (accept) {
      y = half_right.value;
      t += h;
      ++result.steps;
    } else {
      ++result.rejected_steps;
    }
    const double factor =
        scaled_error == 0.0
            ? 4.0
            : std::clamp(0.9 * std::pow(scaled_error, -1.0 / 3.0), 0.2,
                         accept ? 4.0 : 1.0);
    magnitude = std::min(options.maximum_step, magnitude * factor);
    if (options.minimum_step > 0.0 && magnitude < options.minimum_step) {
      if (!accept)
        ::pluss::throw_error(
            "subsp::math::ode::fluxa_linear: required step is below "
            "minimum_step");
      magnitude = options.minimum_step;
    }
  }
  ::pluss::throw_error(
      "subsp::math::ode::fluxa_linear: adaptive step budget exhausted");
}

inline OdeEventResult fluxa_until(
    const OdeFn &f, const OdeEventFn &event, const Series &y0, double t0,
    double t1, const Series &theta = pluss::series::undefined(),
    const OdeOptions &options = {}, const OdeEventOptions &event_options = {}) {
  if (!f || !event)
    ::pluss::throw_error("subsp::math::ode::fluxa_until: derivative and event "
                         "callbacks are required");
  detail::require_defined_(y0, "fluxa_until");
  if (options.method != OdeMethod::RK4)
    ::pluss::throw_error(
        "subsp::math::ode::fluxa_until: only fixed RK4 event refinement is "
        "currently supported");
  if (options.steps <= 0)
    ::pluss::throw_error("subsp::math::ode::fluxa_until: steps must be > 0");
  if (!(event_options.time_tolerance > 0.0) ||
      !std::isfinite(event_options.time_tolerance) ||
      !(event_options.value_tolerance >= 0.0) ||
      !std::isfinite(event_options.value_tolerance) ||
      event_options.max_refinements < 0) {
    ::pluss::throw_error(
        "subsp::math::ode::fluxa_until: invalid event refinement options");
  }

  OdeEventResult result;
  result.y_final = y0;
  result.t_final = t0;
  double left_value =
      detail::event_scalar_(event, t0, y0, theta, &result.event_value);
  ++result.event_evaluations;
  if (std::abs(left_value) <= event_options.value_tolerance) {
    result.triggered = true;
    result.converged = true;
    return result;
  }

  const double h = (t1 - t0) / static_cast<double>(options.steps);
  Series left_state = y0;
  double left_time = t0;
  for (std::int64_t step = 0; step < options.steps; ++step) {
    const double right_time = left_time + h;
    Series right_state = detail::rk4_step_(f, left_time, h, left_state, theta);
    result.function_evaluations += 4;
    Series right_raw;
    const double right_value = detail::event_scalar_(
        event, right_time, right_state, theta, &right_raw);
    ++result.event_evaluations;
    result.steps = step + 1;

    if (detail::crosses_(left_value, right_value, event_options.direction)) {
      double lo_time = left_time;
      double hi_time = right_time;
      Series lo_state = left_state;
      Series hi_state = std::move(right_state);
      double lo_value = left_value;
      double hi_value = right_value;
      Series hi_raw = std::move(right_raw);

      for (std::int64_t refinement = 0;
           refinement < event_options.max_refinements &&
           std::abs(hi_time - lo_time) > event_options.time_tolerance &&
           std::abs(hi_value) > event_options.value_tolerance;
           ++refinement) {
        const double mid_time = lo_time + (hi_time - lo_time) * 0.5;
        Series mid_state =
            detail::rk4_step_(f, lo_time, mid_time - lo_time, lo_state, theta);
        result.function_evaluations += 4;
        Series mid_raw;
        const double mid_value =
            detail::event_scalar_(event, mid_time, mid_state, theta, &mid_raw);
        ++result.event_evaluations;
        if (std::abs(mid_value) <= event_options.value_tolerance ||
            detail::crosses_(lo_value, mid_value, event_options.direction)) {
          hi_time = mid_time;
          hi_state = std::move(mid_state);
          hi_value = mid_value;
          hi_raw = std::move(mid_raw);
        } else {
          lo_time = mid_time;
          lo_state = std::move(mid_state);
          lo_value = mid_value;
        }
      }
      result.y_final = std::move(hi_state);
      result.t_final = hi_time;
      result.event_value = std::move(hi_raw);
      result.triggered = true;
      result.converged = true;
      return result;
    }

    left_time = right_time;
    left_state = std::move(right_state);
    left_value = right_value;
    result.event_value = std::move(right_raw);
  }

  result.y_final = std::move(left_state);
  result.t_final = left_time;
  result.converged = true;
  return result;
}

// Adjoint-sensitivity solve: forward-solves the ODE (storing the
// trajectory's y values at every step boundary), then computes
// d(loss)/d(y0) and, if `theta` is defined, d(loss)/d(theta) -- given a
// seed gradient dL/dy(t1) (`grad_y_final`) -- by walking the trajectory
// BACKWARD and computing each step's local vector-Jacobian product via an
// ordinary `.reverse()` call on a freshly-tracked leaf (see the module
// comment above for why "freshly-tracked": reusing a stored trajectory
// value across multiple, separate reverse-mode passes needs the same
// leaf-severing discipline Septima's own flatten/unflatten boundary uses,
// PlussAI/NodeK/NN/optim/optim_base.h's `assign_parameter_`, etc.).
inline OdeAdjointResult fluxa_adjoint(const OdeFn &f, const Series &y0,
                                      double t0, double t1, const Series &theta,
                                      const Series &grad_y_final,
                                      const OdeOptions &options = {}) {
  if (!f) {
    ::pluss::throw_error("subsp::math::ode::fluxa_adjoint: right-hand-side "
                         "callback is required");
  }
  detail::require_defined_(y0, "fluxa_adjoint");
  detail::require_defined_(grad_y_final, "fluxa_adjoint");
  if (options.method != OdeMethod::RK4)
    ::pluss::throw_error(
        "subsp::math::ode::fluxa_adjoint: the checkpointed discrete adjoint "
        "is defined for fixed RK4 only");
  if (options.steps <= 0) {
    ::pluss::throw_error("subsp::math::ode::fluxa_adjoint: steps must be > 0");
  }
  const bool has_theta = theta.defined();

  const double h = (t1 - t0) / static_cast<double>(options.steps);

  // Forward pass: store every step boundary's y (checkpointed -- see the
  // module comment for the O(steps)-vs-O(1) memory tradeoff this implies).
  std::vector<Series> trajectory;
  trajectory.reserve(static_cast<std::size_t>(options.steps) + 1);
  trajectory.push_back(y0);
  {
    Series y = y0;
    double t = t0;
    for (std::int64_t i = 0; i < options.steps; ++i) {
      y = detail::rk4_step_(f, t, h, y, theta);
      t += h;
      trajectory.push_back(y);
    }
  }

  OdeAdjointResult result;
  result.y_final = trajectory.back();
  result.forward_steps = options.steps;

  // Backward pass.
  Series a = grad_y_final;
  Series theta_grad = pluss::series::undefined(); // accumulated lazily; stays
                                                  // undefined if !has_theta
  double t = t1;
  for (std::int64_t i = options.steps; i > 0; --i) {
    t -= h;

    Series y_leaf = trajectory[static_cast<std::size_t>(i - 1)].detach();
    pluss::codem::series_tape::clear_source_link(y_leaf);
    auto ty = pluss::subsp::math::diff::trace(y_leaf, /*tracks_slope=*/true);
    const Series y_tracked = ty.value();

    Series theta_tracked = pluss::series::undefined();
    pluss::subsp::math::diff::Trace ttheta;
    if (has_theta) {
      Series theta_leaf = theta.detach();
      pluss::codem::series_tape::clear_source_link(theta_leaf);
      ttheta =
          pluss::subsp::math::diff::trace(theta_leaf, /*tracks_slope=*/true);
      theta_tracked = ttheta.value();
    }

    const Series y_next = detail::rk4_step_(f, t, h, y_tracked, theta_tracked);
    y_next.reverse(a);
    a = y_tracked.sensitivity();
    if (has_theta) {
      const Series step_grad = theta_tracked.sensitivity();
      theta_grad = theta_grad.defined() ? theta_grad.add(step_grad) : step_grad;
    }
  }

  result.grad_y0 = a;
  result.grad_theta = theta_grad;
  result.backward_steps = options.steps;
  return result;
}

} // namespace pluss::subsp::math::ode
