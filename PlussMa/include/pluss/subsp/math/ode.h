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
//  - `fluxa()` (forward-only) uses fixed-step RK4. Deliberately NOT an
//    adaptive method (no embedded-pair step-size control, unlike
//    pluss::integ::Quadra's own step-doubling) -- a real, honest scope
//    limit for this first pass, not a claim of production-grade
//    adaptivity.
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

#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace pluss::subsp::math::ode {

// dy/dt = f(t, y, theta). `t` is a plain double (ODE time is not part of
// the Series/autodiff system -- only y and theta are). `theta` may be an
// undefined Series if the right-hand side has no parameters the caller
// wants gradients against (everything it needs already captured directly
// in the closure).
using OdeFn = std::function<Series(double, const Series&, const Series&)>;

enum class OdeMethod : std::uint8_t {
  RK4 = 0,
};

struct OdeOptions {
  OdeMethod method{OdeMethod::RK4};
  std::int64_t steps{100};
};

struct OdeResult {
  Series y_final = pluss::series::undefined();
  std::int64_t steps{0};
  std::int64_t function_evaluations{0};
  bool converged{false};
};

struct OdeAdjointResult {
  Series y_final = pluss::series::undefined();
  Series grad_y0 = pluss::series::undefined();
  Series grad_theta = pluss::series::undefined();   // undefined if `theta` was undefined
  std::int64_t forward_steps{0};
  std::int64_t backward_steps{0};
};

namespace detail {

inline void require_defined_(const Series& x, const char* op) {
  if (!x.defined()) {
    ::pluss::throw_error(std::string("subsp::math::ode::") + op + ": expected defined Series");
  }
}

// One classical RK4 step (Series-native, differentiable for free through
// f's own already-differentiable body -- no custom backward rule here
// either, the same "compose, inherit" property everywhere else in this
// file has).
inline Series rk4_step_(const OdeFn& f, double t, double h, const Series& y, const Series& theta) {
  const Series k1 = f(t, y, theta);
  const Series k2 = f(t + h * 0.5, y.add(k1.mul(h * 0.5)), theta);
  const Series k3 = f(t + h * 0.5, y.add(k2.mul(h * 0.5)), theta);
  const Series k4 = f(t + h, y.add(k3.mul(h)), theta);
  return y.add(k1.add(k2.mul(2.0)).add(k3.mul(2.0)).add(k4).mul(h / 6.0));
}

}  // namespace detail

inline OdeResult fluxa(const OdeFn& f, const Series& y0, double t0, double t1, const Series& theta = pluss::series::undefined(),
                       const OdeOptions& options = {}) {
  if (!f) {
    ::pluss::throw_error("subsp::math::ode::fluxa: right-hand-side callback is required");
  }
  detail::require_defined_(y0, "fluxa");
  if (options.steps <= 0) {
    ::pluss::throw_error("subsp::math::ode::fluxa: steps must be > 0");
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
inline OdeAdjointResult fluxa_adjoint(const OdeFn& f, const Series& y0, double t0, double t1, const Series& theta,
                                     const Series& grad_y_final, const OdeOptions& options = {}) {
  if (!f) {
    ::pluss::throw_error("subsp::math::ode::fluxa_adjoint: right-hand-side callback is required");
  }
  detail::require_defined_(y0, "fluxa_adjoint");
  detail::require_defined_(grad_y_final, "fluxa_adjoint");
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
  Series theta_grad = pluss::series::undefined();  // accumulated lazily; stays undefined if !has_theta
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
      ttheta = pluss::subsp::math::diff::trace(theta_leaf, /*tracks_slope=*/true);
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

}  // namespace pluss::subsp::math::ode
