#pragma once

// pluss::subsp::math::sde -- Series-native stochastic differential equation
// simulation and stochastic (Ito/Stratonovich) integration, with pathwise
// adjoint-sensitivity gradients.
//
// Naming: **Fluctua** (Latin, "a fluctuating/wavering thing") is the
// forward/adjoint SDE solver pair (fluctua/fluctua_adjoint), the direct
// stochastic sibling of pluss::ode's **Fluxa** (deterministic flow) --
// same Latin-adjective family this program's numerical modules already
// use (Septima/Optima/Minimax under pluss::optim; Integra/Quadra/
// Aleatoria/Analytica under pluss::integ; Fluxa under pluss::ode). The
// standalone stochastic-integral primitives below (ito_integral/
// stratonovich_integral) keep their standard field names rather than a
// Latin pseudonym -- "Ito"/"Stratonovich" are already the universally
// standard terms (named after their originators, like "Gauss-Legendre"),
// not generic descriptive words the way quadrature/analytic/random are.
//
// Why this is its OWN module, not a mode flag on anything else: this
// program's stochastic-calculus surface was, before this file, exactly
// zero -- confirmed by repo-wide search. It is tempting to think of a
// stochastic integral as "quadra/aleatoria but with random nodes," or of
// an SDE as "an ODE with a noise term," but both framings are wrong in a
// way that would silently produce the wrong number if taken literally:
//
//   - An Ito integral integral_0^T f(t, W_t) dW_t is NOT a Riemann-
//     Stieltjes integral along a Brownian sample path -- it can't be,
//     because Brownian paths have unbounded variation, so the pathwise
//     Riemann-Stieltjes limit simply does not converge. Ito's actual
//     construction is an L^2 limit over simple predictable processes, a
//     measure-theoretic definition, not a domain-partitioning one. This
//     is also why the discrete sum used to APPROXIMATE it must evaluate
//     the integrand at the LEFT endpoint of every subinterval (the
//     integrand must be "non-anticipating" / adapted -- it cannot know
//     the future increment it is about to be multiplied by); using the
//     midpoint or trapezoidal average instead gives a DIFFERENT, also
//     well-defined object (the Stratonovich integral), not an
//     approximation error -- see stratonovich_integral() below, and the
//     module's test file for the exact discrete identities that pin both
//     conventions down independently.
//   - Differentiating an SDE's solution with respect to its own
//     parameters (a "stochastic Greek") is NOT the same operation as
//     ordinary autodiff through a fixed computation graph, and it is NOT
//     Malliavin calculus either (a genuinely separate, harder
//     construction differentiating a random variable with respect to the
//     underlying Wiener path itself, in a weak/Hilbert-space sense -- not
//     attempted here). What IS built here is the PATHWISE (aka
//     "reparameterization trick") sensitivity: fix the sampled Wiener
//     increments as ordinary constants (not re-sampled, not
//     differentiated), which makes the discretized SDE solve an ordinary
//     deterministic function of (y0, theta, the fixed noise draws) --
//     and THAT function is differentiated by Pluss's already-trusted
//     reverse-mode engine, exactly the same checkpointed-adjoint
//     architecture pluss::ode::fluxa_adjoint already uses, just with an
//     extra diffusion term per step. This is the same technique used for
//     pathwise Greeks in finance and the reparameterization trick in
//     variational inference -- but it is NOT a general-purpose stochastic
//     derivative: it breaks down for genuinely discontinuous payoffs,
//     where Malliavin/likelihood-ratio methods are needed instead (not
//     built here, a real, separate, documented gap).
//
// Scope/tradeoffs, stated plainly (mirroring pluss::ode's own discipline):
//  - fluctua() uses fixed-step Euler-Maruyama only (strong order 0.5).
//    Milstein (strong order 1.0, needs d(sigma)/dy) is real, useful,
//    future work -- not built here, to keep this first pass to one
//    well-verified scheme rather than several unverified ones.
//  - Diffusion is elementwise/uncorrelated: `sigma(t, y, theta)` must be
//    the SAME shape as `y`, and drives one independent scalar Wiener
//    increment per entry of `y`. This is deliberately NOT a general
//    multi-factor SDE with a correlation/Cholesky structure across `y`'s
//    own entries -- a real, separate piece of work, not attempted here.
//    It DOES, for free, support batching many independent SDE paths in
//    one call (y0 with a leading batch axis), exactly like
//    pluss::integ::quadra's batched integrals -- each batch entry is an
//    independent realization, not a correlated dimension.
//  - fluctua_adjoint()'s backward pass is the same checkpointed discrete
//    adjoint pluss::ode::fluxa_adjoint uses: the forward trajectory's y
//    values AND the realized Wiener increments are stored (O(steps)
//    memory), then walked backward computing each step's local
//    vector-Jacobian product via an ordinary `.reverse()` call on a
//    freshly-tracked leaf. The realized noise draws are treated as fixed
//    constants in the backward pass too (this is exactly what "pathwise"
//    means) -- they are never re-tracked or differentiated.
//  - ito_integral()/stratonovich_integral() operate against an explicit,
//    already-simulated WienerPath (see simulate_wiener_path()) rather
//    than an implicit one, so callers can reuse the SAME realized path
//    across multiple integrands or cross-checks -- mirroring why quadra's
//    nodes are computed once and reused for all of aleatoria's
//    diagnostics.
//  - Only a flat/elementwise `y`/`theta` state is supported (matching
//    pluss::ode::OdeFn's own "flatten your own problem into one Series"
//    convention). A caller with structured state flattens/unflattens it
//    themselves.

#include "pluss/subsp/math/diff.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace pluss::subsp::math::sde {

// dY = mu(t, Y, theta) dt + sigma(t, Y, theta) dW. `t` is a plain double
// (as in pluss::ode::OdeFn, time is not part of the Series/autodiff
// system). `sigma`'s output must be the same shape as `y` -- see the
// module comment above for why this is elementwise/uncorrelated diffusion,
// not a general multi-factor covariance structure.
using DriftFn = std::function<Series(double, const Series&, const Series&)>;
using DiffusionFn = std::function<Series(double, const Series&, const Series&)>;

// y_final -> a SCALAR (or single-element) loss. Used by the fluctua_adjoint
// overload below that auto-derives the seed gradient, so a caller doesn't
// have to hand-differentiate their own downstream transform of y_final
// first -- see that overload's own comment for why this was worth adding.
using LossFn = std::function<Series(const Series&)>;

enum class SdeMethod : std::uint8_t {
  EulerMaruyama = 0,
};

struct SdeOptions {
  SdeMethod method{SdeMethod::EulerMaruyama};
  std::int64_t steps{100};
  std::uint64_t seed{0};
  // Deliberately NOT dtype/platform fields (unlike QuadraOptions/
  // AleatoriaOptions): fluctua() always has y0 in hand, so the simulated
  // noise is generated in y0's own dtype/platform directly -- avoiding a
  // silent-mismatch footgun a separate, independently-defaulted dtype/
  // platform pair would risk.
};

struct SdeResult {
  Series y_final = pluss::series::undefined();
  // W(t1) - W(t0), same shape as y0 -- the realized Wiener increment this
  // specific path used, exposed so a caller can cross-check against a
  // closed-form SDE solution (e.g. GBM's) that is a function of the SAME
  // realized path, not just the scheme's numeric output in isolation.
  Series wiener_final = pluss::series::undefined();
  std::int64_t steps{0};
  bool converged{false};
};

struct SdeAdjointResult {
  Series y_final = pluss::series::undefined();
  // W(t1) - W(t0), same shape as y0 -- mirrors SdeResult::wiener_final,
  // exposed for the same reason (auditing/reusing the realized path).
  Series wiener_final = pluss::series::undefined();
  Series grad_y0 = pluss::series::undefined();
  Series grad_theta = pluss::series::undefined();  // undefined if `theta` was undefined
  std::int64_t forward_steps{0};
  std::int64_t backward_steps{0};
};

namespace detail {

inline void require_defined_(const Series& x, const char* op) {
  if (!x.defined()) {
    ::pluss::throw_error(std::string("subsp::math::sde::") + op + ": expected defined Series");
  }
}

// One flat batched draw of Wiener increments: shape [steps] + like's own
// shape, each entry ~ N(0, dt) i.i.d. -- one independent scalar increment
// per (step, batch-entry) pair. Generated as ONE randn() call (not one per
// step), matching this program's "batched, not per-point" discipline.
inline Series wiener_increments_(const Series& like, std::int64_t steps, double dt, std::uint64_t seed) {
  std::vector<std::int64_t> sizes;
  sizes.push_back(steps);
  for (std::int64_t d = 0; d < like.dim(); ++d) sizes.push_back(like.size(d));
  Series z = pluss::series::randn(pluss::msharp::RangeView::Int(sizes),
                                  Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()), seed);
  return z.mul(std::sqrt(dt));
}

inline Series em_step_(const DriftFn& mu, const DiffusionFn& sigma, double t, const Series& y, const Series& theta,
                       double h, const Series& dw) {
  return y.add(mu(t, y, theta).mul(h)).add(sigma(t, y, theta).mul(dw));
}

}  // namespace detail

inline SdeResult fluctua(const DriftFn& mu, const DiffusionFn& sigma, const Series& y0, double t0, double t1,
                         const Series& theta = pluss::series::undefined(), const SdeOptions& options = {}) {
  if (!mu) {
    ::pluss::throw_error("subsp::math::sde::fluctua: drift callback is required");
  }
  if (!sigma) {
    ::pluss::throw_error("subsp::math::sde::fluctua: diffusion callback is required");
  }
  detail::require_defined_(y0, "fluctua");
  if (options.steps <= 0) {
    ::pluss::throw_error("subsp::math::sde::fluctua: steps must be > 0");
  }

  const double h = (t1 - t0) / static_cast<double>(options.steps);
  const Series dw = detail::wiener_increments_(y0, options.steps, h, options.seed);

  SdeResult result;
  Series y = y0;
  Series w_accum = Series::zeros(y0.sizes(), y0.scalar_type(), y0.platform());
  double t = t0;
  for (std::int64_t i = 0; i < options.steps; ++i) {
    const Series dw_i = dw.select(0, i);
    y = detail::em_step_(mu, sigma, t, y, theta, h, dw_i);
    w_accum = w_accum.add(dw_i);
    t += h;
  }
  result.y_final = y;
  result.wiener_final = w_accum;
  result.steps = options.steps;
  result.converged = true;
  return result;
}

// Pathwise adjoint-sensitivity solve: forward-solves the SDE (storing the
// trajectory's y values AND the realized Wiener increments at every step),
// then computes d(loss)/d(y0) and, if `theta` is defined, d(loss)/d(theta)
// given a seed gradient dL/dY(t1) (`grad_y_final`). The realized noise is
// held fixed (treated as an ordinary constant, never re-tracked or
// differentiated) in both passes -- this fixed-noise treatment is exactly
// what makes this a PATHWISE derivative (see the module comment).
//
// `grad_y_final` must be supplied by the CALLER -- this overload does not
// automatically differentiate through any transform applied to y_final
// afterward (e.g. wanting d[exp(lambda*y_final)]/dtheta means the caller
// must pass grad_y_final = lambda*exp(lambda*y_final) themselves, computed
// however they like, e.g. via their own small trace()+reverse() call).
// That's fine when you already have the seed in hand, but it was a real,
// avoidable ergonomic gap for the common case of "I have a scalar loss
// defined in terms of y_final, just differentiate through it" -- the
// fluctua_adjoint(..., LossFn, ...) overload right below exists
// specifically to remove that gap; prefer it unless you already have
// grad_y_final computed some other way.
inline SdeAdjointResult fluctua_adjoint(const DriftFn& mu, const DiffusionFn& sigma, const Series& y0, double t0,
                                        double t1, const Series& theta, const Series& grad_y_final,
                                        const SdeOptions& options = {}) {
  if (!mu) {
    ::pluss::throw_error("subsp::math::sde::fluctua_adjoint: drift callback is required");
  }
  if (!sigma) {
    ::pluss::throw_error("subsp::math::sde::fluctua_adjoint: diffusion callback is required");
  }
  detail::require_defined_(y0, "fluctua_adjoint");
  detail::require_defined_(grad_y_final, "fluctua_adjoint");
  if (options.steps <= 0) {
    ::pluss::throw_error("subsp::math::sde::fluctua_adjoint: steps must be > 0");
  }
  const bool has_theta = theta.defined();

  const double h = (t1 - t0) / static_cast<double>(options.steps);
  const Series dw = detail::wiener_increments_(y0, options.steps, h, options.seed);

  // Forward pass: store every step boundary's y (checkpointed, same
  // O(steps)-memory tradeoff pluss::ode::fluxa_adjoint documents).
  std::vector<Series> trajectory;
  trajectory.reserve(static_cast<std::size_t>(options.steps) + 1);
  trajectory.push_back(y0);
  {
    Series y = y0;
    double t = t0;
    for (std::int64_t i = 0; i < options.steps; ++i) {
      y = detail::em_step_(mu, sigma, t, y, theta, h, dw.select(0, i));
      t += h;
      trajectory.push_back(y);
    }
  }

  SdeAdjointResult result;
  result.y_final = trajectory.back();
  result.wiener_final = dw.sum(0);
  result.forward_steps = options.steps;

  // Backward pass -- identical structure to pluss::ode::fluxa_adjoint's
  // own backward pass, just replaying em_step_ (drift*h + diffusion*dw)
  // instead of rk4_step_ at each step. `dw_i` is detached/constant, so it
  // never receives or needs a sensitivity of its own.
  Series a = grad_y_final;
  Series theta_grad = pluss::series::undefined();
  double t = t1;
  for (std::int64_t i = options.steps; i > 0; --i) {
    t -= h;
    const Series dw_i = dw.select(0, i - 1);

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

    const Series y_next = detail::em_step_(mu, sigma, t, y_tracked, theta_tracked, h, dw_i);
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

// Same as above, but auto-derives the seed gradient from a caller-supplied
// SCALAR loss function of y_final, instead of requiring the caller to
// compute d(loss)/d(y_final) themselves first. Internally: runs an
// ordinary fluctua() forward pass to get y_final (fluctua() is
// deterministic given the same mu/sigma/y0/t0/t1/theta/options, so this is
// bit-identical to the trajectory the seed-based overload below will
// independently recompute -- one extra forward pass, negligible next to
// the backward pass's own cost), differentiates `loss` through a freshly-
// traced copy of that y_final (same "detach + clear_source_link + trace"
// discipline as the backward pass itself uses per-step), and feeds the
// resulting gradient into the existing seed-based overload unchanged. This
// composes with ANY differentiable transform of y_final, not just a fixed
// list -- `loss` can be arbitrarily complex Series code, exactly like
// `quadra`'s own integrand.
//
// `loss` MUST return a scalar (or single-element) Series: `.reverse()` is
// called on it with no explicit seed, so a batched fluctua() call (y0 with
// a leading batch axis, many independent paths in one call) needs `loss`
// to reduce across that batch itself (e.g. `.mean()`), the same way any
// scalar training loss would.
inline SdeAdjointResult fluctua_adjoint(const DriftFn& mu, const DiffusionFn& sigma, const Series& y0, double t0,
                                        double t1, const Series& theta, const LossFn& loss,
                                        const SdeOptions& options = {}) {
  if (!loss) {
    ::pluss::throw_error("subsp::math::sde::fluctua_adjoint: loss callback is required");
  }
  const SdeResult fwd = fluctua(mu, sigma, y0, t0, t1, theta, options);

  Series y_leaf = fwd.y_final.detach();
  pluss::codem::series_tape::clear_source_link(y_leaf);
  auto ty = pluss::subsp::math::diff::trace(y_leaf, /*tracks_slope=*/true);
  const Series loss_value = loss(ty.value());
  detail::require_defined_(loss_value, "fluctua_adjoint");
  loss_value.reverse();
  const Series grad_y_final = ty.value().sensitivity();

  return fluctua_adjoint(mu, sigma, y0, t0, t1, theta, grad_y_final, options);
}

// ---------------------------------------------------------------------
// Standalone stochastic integrals against an explicit, already-simulated
// scalar Wiener path -- see the module comment for why Ito and
// Stratonovich are genuinely different objects, not two ways of
// approximating the same one.
// ---------------------------------------------------------------------

// f(t, W_t) -> Series. `like` (passed to simulate_wiener_path) fixes the
// batch shape/dtype/platform -- many independent scalar Wiener paths can
// be simulated and integrated in one batched call, exactly like fluctua's
// own batching.
using WienerIntegrandFn = std::function<Series(double, const Series&)>;

struct WienerPath {
  // Shape [steps + 1] + like's shape. w[0] == 0 (W(t0) := 0); w[i] is
  // W(t_i) - W(t0) for t_i = t0 + i*h.
  Series w = pluss::series::undefined();
  std::vector<double> times{};
  double t0{0.0};
  double t1{0.0};
  std::int64_t steps{0};
};

struct StochasticIntegralResult {
  Series value = pluss::series::undefined();
  // sum_i (W(t_{i+1}) - W(t_i))^2 for the SAME path -- the realized
  // quadratic variation, exposed because it is exactly the quantity that
  // makes the Ito vs Stratonovich difference concrete (their difference
  // equals HALF of this, exactly, for any step count -- see the test
  // file's exact discrete-identity checks).
  Series realized_quadratic_variation = pluss::series::undefined();
};

inline WienerPath simulate_wiener_path(const Series& like, double t0, double t1, std::int64_t steps,
                                       std::uint64_t seed) {
  if (steps <= 0) {
    ::pluss::throw_error("subsp::math::sde::simulate_wiener_path: steps must be > 0");
  }
  const double h = (t1 - t0) / static_cast<double>(steps);
  const Series dw = detail::wiener_increments_(like, steps, h, seed);
  const Series zero = Series::zeros(like.sizes(), like.scalar_type(), like.platform()).unsqueeze(0);
  const Series w = zero.unite({dw.cumsum(0)}, 0);

  WienerPath path;
  path.w = w;
  path.t0 = t0;
  path.t1 = t1;
  path.steps = steps;
  path.times.reserve(static_cast<std::size_t>(steps) + 1);
  for (std::int64_t i = 0; i <= steps; ++i) path.times.push_back(t0 + h * static_cast<double>(i));
  return path;
}

namespace detail {

inline Series quadratic_variation_(const WienerPath& path) {
  const Series dw = path.w.diff(1, 0);
  return dw.mul(dw).sum(0);
}

}  // namespace detail

// Discrete Ito sum: sum_i f(t_i, W_{t_i}) * (W_{t_{i+1}} - W_{t_i}) --
// LEFT-endpoint evaluation, the defining, non-negotiable feature of the
// Ito convention (the integrand must not anticipate the increment it is
// about to be multiplied by). See the module comment for why this is not
// interchangeable with the Stratonovich convention below.
inline StochasticIntegralResult ito_integral(const WienerIntegrandFn& f, const WienerPath& path) {
  if (!f) {
    ::pluss::throw_error("subsp::math::sde::ito_integral: integrand callback is required");
  }
  Series total = pluss::series::undefined();
  for (std::int64_t i = 0; i < path.steps; ++i) {
    const Series w_i = path.w.select(0, i);
    const Series dw_i = path.w.select(0, i + 1).sub(w_i);
    const Series term = f(path.times[static_cast<std::size_t>(i)], w_i).mul(dw_i);
    total = total.defined() ? total.add(term) : term;
  }
  StochasticIntegralResult result;
  result.value = total;
  result.realized_quadratic_variation = detail::quadratic_variation_(path);
  return result;
}

// Discrete Stratonovich sum: sum_i 0.5*(f(t_i, W_{t_i}) + f(t_{i+1},
// W_{t_{i+1}})) * (W_{t_{i+1}} - W_{t_i}) -- trapezoidal-averaged
// evaluation. Unlike Ito's, this convention obeys ordinary calculus's
// chain rule, which is exactly why it disagrees with ito_integral() by a
// term that is NOT an approximation error (see the test file).
inline StochasticIntegralResult stratonovich_integral(const WienerIntegrandFn& f, const WienerPath& path) {
  if (!f) {
    ::pluss::throw_error("subsp::math::sde::stratonovich_integral: integrand callback is required");
  }
  Series total = pluss::series::undefined();
  for (std::int64_t i = 0; i < path.steps; ++i) {
    const Series w_i = path.w.select(0, i);
    const Series w_ip1 = path.w.select(0, i + 1);
    const Series dw_i = w_ip1.sub(w_i);
    const Series f_i = f(path.times[static_cast<std::size_t>(i)], w_i);
    const Series f_ip1 = f(path.times[static_cast<std::size_t>(i + 1)], w_ip1);
    const Series term = f_i.add(f_ip1).mul(0.5).mul(dw_i);
    total = total.defined() ? total.add(term) : term;
  }
  StochasticIntegralResult result;
  result.value = total;
  result.realized_quadratic_variation = detail::quadratic_variation_(path);
  return result;
}

}  // namespace pluss::subsp::math::sde
