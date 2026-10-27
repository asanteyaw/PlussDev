#pragma once

// pluss::subsp::math::sde -- Series-native stochastic differential equation
// simulation and stochastic (Ito/Stratonovich) integration, with pathwise
// adjoint-sensitivity gradients.
//
// Naming: **Fluctua** (Latin, "a fluctuating/wavering thing") is the
// forward/adjoint SDE solver pair (fluctua/fluctua_adjoint), the direct
// stochastic sibling of pluss::subsp::math::ode's **Fluxa** (deterministic flow) --
// same Latin-adjective family this program's numerical modules already
// use (Septima/Optima/Minimax under pluss::subsp::math::optim; Integra/Quadra/
// Aleatoria/Analytica under pluss::subsp::math::integ; Fluxa under pluss::subsp::math::ode). The
// standalone stochastic-integral primitives below (ito_integral/
// strat_integral) keep their standard field names rather than a
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
//     approximation error -- see strat_integral() below, and the
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
//     architecture pluss::subsp::math::ode::fluxa_adjoint already uses, just with an
//     extra diffusion term per step. This is the same technique used for
//     pathwise Greeks in finance and the reparameterization trick in
//     variational inference -- but it is NOT a general-purpose stochastic
//     derivative: it breaks down for genuinely discontinuous payoffs,
//     where Malliavin/likelihood-ratio methods are needed instead (not
//     built here, a real, separate, documented gap).
//
// Scope/tradeoffs, stated plainly (mirroring pluss::subsp::math::ode's own discipline):
//  - fluctua() uses a fixed step. Schemes: Euler-Maruyama (Ito, strong order
//    0.5), Milstein (Ito, strong order 1.0 for diagonal noise) and a
//    Stratonovich Heun scheme (see SdeMethod).
//  - Diffusion is elementwise/uncorrelated: `sigma(t, y, theta)` must be
//    the SAME shape as `y`, and drives one independent scalar Wiener
//    increment per entry of `y`. This is deliberately NOT a general
//    multi-factor SDE with a correlation/Cholesky structure across `y`'s
//    own entries -- a real, separate piece of work, not attempted here.
//    It DOES, for free, support batching many independent SDE paths in
//    one call (y0 with a leading batch axis), exactly like
//    pluss::subsp::math::integ::quadra's batched integrals -- each batch entry is an
//    independent realization, not a correlated dimension.
//  - fluctua_adjoint()'s backward pass is the same checkpointed discrete
//    adjoint pluss::subsp::math::ode::fluxa_adjoint uses: the forward trajectory's y
//    values AND the realized Wiener increments are stored (O(steps)
//    memory), then walked backward computing each step's local
//    vector-Jacobian product via an ordinary `.reverse()` call on a
//    freshly-tracked leaf. The realized noise draws are treated as fixed
//    constants in the backward pass too (this is exactly what "pathwise"
//    means) -- they are never re-tracked or differentiated.
//  - ito_integral()/strat_integral() operate against an explicit,
//    already-simulated WienerPath (see simulate_wiener_path()) rather
//    than an implicit one, so callers can reuse the SAME realized path
//    across multiple integrands or cross-checks -- mirroring why quadra's
//    nodes are computed once and reused for all of aleatoria's
//    diagnostics.
//  - Only a flat/elementwise `y`/`theta` state is supported (matching
//    pluss::subsp::math::ode::OdeFn's own "flatten your own problem into one Series"
//    convention). A caller with structured state flattens/unflattens it
//    themselves.

#include "pluss/subsp/math/diff.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace pluss::subsp::math::sde {

// dY = mu(t, Y, theta) dt + sigma(t, Y, theta) dW. `t` is a plain double
// (as in pluss::subsp::math::ode::OdeFn, time is not part of the Series/autodiff
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

// EulerMaruyama: Ito SDE, strong order 0.5.
// Milstein:      Ito SDE, strong order 1.0 for diagonal noise; adds 1/2 sigma sigma' (dW^2 - h), with
//                sigma' = d sigma_i / d y_i taken elementwise by autodiff. sigma_i must therefore depend on y_i
//                only (not on the other components), as the module's elementwise-diffusion convention already
//                assumes.
// StratonovichHeun: the SDE is read in the STRATONOVICH sense (mu and sigma define dY = mu dt + sigma o dW),
//                predictor-corrector with the trapezoidal average of the coefficients; strong order 1.0 for
//                scalar or commutative noise. Convert a drift between the two readings with
//                `y.calc.diff.strat_drift(...)` / `ito_drift(...)`.
enum class SdeMethod : std::uint8_t {
  EulerMaruyama = 0,
  Milstein = 1,
  StratonovichHeun = 2,
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

// d sigma_i / d y_i, elementwise. When `y` is itself being differentiated (the adjoint replay) the derivative is
// returned with its graph attached to `y` and `theta`, so the step stays differentiable through it; otherwise it
// is computed on a throwaway leaf.
inline Series sigma_prime_(const DiffusionFn& sigma, double t, const Series& y, const Series& theta) {
  const bool attached = y.tracks_slope();
  Series leaf = y;
  if (!attached) {
    leaf = y.detach();
    pluss::codem::series_tape::clear_source_link(leaf);
    pluss::subsp::math::diff::detail::set_tracks_slope_(leaf, true);
  }
  const Series s = sigma(t, leaf, theta);
  Series zero = Series::zeros(y.sizes(), y.scalar_type(), y.platform());
  if (!pluss::subsp::math::diff::detail::has_reverse_path_(s)) return zero;
  pluss::codem::series_tape::clear_sensitivity(leaf);
  if (theta.defined() && theta.tracks_slope()) pluss::codem::series_tape::clear_sensitivity(theta);
  s.sum().reverse(/*keep_graph=*/true, /*accumulate_sensitivity=*/false, /*track_higher_order=*/attached);
  Series d = leaf.has_sensitivity() ? leaf.sensitivity() : zero;
  if (!attached) d = d.detach();
  pluss::codem::series_tape::clear_sensitivity(leaf);
  if (theta.defined() && theta.tracks_slope()) pluss::codem::series_tape::clear_sensitivity(theta);
  return d;
}

inline Series milstein_step_(const DriftFn& mu, const DiffusionFn& sigma, double t, const Series& y,
                             const Series& theta, double h, const Series& dw) {
  const Series s = sigma(t, y, theta);
  const Series ds = sigma_prime_(sigma, t, y, theta);
  return y.add(mu(t, y, theta).mul(h))
      .add(s.mul(dw))
      .add(s.mul(ds).mul(dw.mul(dw).sub(h)).mul(0.5));
}

inline Series heun_step_(const DriftFn& mu, const DiffusionFn& sigma, double t, const Series& y, const Series& theta,
                         double h, const Series& dw) {
  const Series m0 = mu(t, y, theta);
  const Series s0 = sigma(t, y, theta);
  const Series predictor = y.add(m0.mul(h)).add(s0.mul(dw));
  const Series m1 = mu(t + h, predictor, theta);
  const Series s1 = sigma(t + h, predictor, theta);
  return y.add(m0.add(m1).mul(0.5 * h)).add(s0.add(s1).mul(0.5).mul(dw));
}

inline Series step_(SdeMethod method, const DriftFn& mu, const DiffusionFn& sigma, double t, const Series& y,
                    const Series& theta, double h, const Series& dw) {
  switch (method) {
    case SdeMethod::EulerMaruyama: return em_step_(mu, sigma, t, y, theta, h, dw);
    case SdeMethod::Milstein: return milstein_step_(mu, sigma, t, y, theta, h, dw);
    case SdeMethod::StratonovichHeun: return heun_step_(mu, sigma, t, y, theta, h, dw);
  }
  ::pluss::throw_error("subsp::math::sde: unknown SDE method");
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
    y = detail::step_(options.method, mu, sigma, t, y, theta, h, dw_i);
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
  // O(steps)-memory tradeoff pluss::subsp::math::ode::fluxa_adjoint documents).
  std::vector<Series> trajectory;
  trajectory.reserve(static_cast<std::size_t>(options.steps) + 1);
  trajectory.push_back(y0);
  {
    Series y = y0;
    double t = t0;
    for (std::int64_t i = 0; i < options.steps; ++i) {
      y = detail::step_(options.method, mu, sigma, t, y, theta, h, dw.select(0, i));
      t += h;
      trajectory.push_back(y);
    }
  }

  SdeAdjointResult result;
  result.y_final = trajectory.back();
  result.wiener_final = dw.sum(0);
  result.forward_steps = options.steps;

  // Backward pass -- identical structure to pluss::subsp::math::ode::fluxa_adjoint's
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

    const Series y_next = detail::step_(options.method, mu, sigma, t, y_tracked, theta_tracked, h, dw_i);
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
  // Notes about the input that did not stop the calculation but change what the number means (for example a path
  // whose realized quadratic variation does not match a standard Wiener path). Empty when nothing was found.
  std::vector<std::string> warnings{};
  // Mean over the batch of (realized quadratic variation) / (t1 - t0); about 1 for a standard Wiener path.
  double quadratic_variation_ratio{1.0};
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

namespace detail {

// Validates an explicit Wiener path before it reaches the canonical Series integral, which only sees samples:
// the grid and the samples must agree in length, the grid must be finite and strictly increasing, and the path
// must be finite. (An integrand that is a callback of (t, W_t) cannot anticipate by construction; it never
// sees a later sample. A callback that captured the path and looked ahead would defeat any check.)
inline void validate_path_(const WienerPath& path, const char* op) {
  if (!path.w.defined() || path.w.dim() < 1) {
    ::pluss::throw_error(std::string("subsp::math::sde::") + op + ": the path is undefined");
  }
  if (path.steps <= 0 || path.w.size(0) != path.steps + 1 ||
      static_cast<std::int64_t>(path.times.size()) != path.steps + 1) {
    ::pluss::throw_error(std::string("subsp::math::sde::") + op +
                         ": the path needs steps + 1 samples and steps + 1 grid times");
  }
  for (std::size_t i = 0; i < path.times.size(); ++i) {
    if (!std::isfinite(path.times[i]) || (i > 0 && !(path.times[i] > path.times[i - 1]))) {
      ::pluss::throw_error(std::string("subsp::math::sde::") + op +
                           ": the time grid must be finite and strictly increasing");
    }
  }
  if (!path.w.isfinite().all().to(pluss::pCPU, ScalarType::Bool).item<bool>()) {
    ::pluss::throw_error(std::string("subsp::math::sde::") + op + ": the path contains non-finite samples");
  }
}

// Compares the realized quadratic variation with the elapsed time. For a standard Wiener path the ratio is 1 up to
// a relative sampling error of about sqrt(2 / (steps * batch)); a path with another volatility, a smooth path (ratio
// near 0, where Ito and Stratonovich coincide) or a path with jumps is flagged.
inline void annotate_(StochasticIntegralResult& result, const WienerPath& path) {
  const double elapsed = path.t1 - path.t0;
  const double batch = static_cast<double>(std::max<std::int64_t>(result.realized_quadratic_variation.numel(), 1));
  result.quadratic_variation_ratio = result.realized_quadratic_variation.mean().item<double>() / elapsed;
  const double spread = 6.0 * std::sqrt(2.0 / (static_cast<double>(path.steps) * batch)) + 1e-3;
  if (std::abs(result.quadratic_variation_ratio - 1.0) > spread) {
    result.warnings.push_back(
        "the path's realized quadratic variation is " + std::to_string(result.quadratic_variation_ratio) +
        " times the elapsed time; a standard Wiener path gives about 1, so this is not a standard Brownian path "
        "(another volatility, a smooth path, or jumps) and Ito and Stratonovich values are not the standard ones");
  }
}

// theta = 0: Ito (left endpoint). theta = 1/2: Stratonovich (average of the endpoint values).
inline StochasticIntegralResult stochastic_integral_(const WienerIntegrandFn& f, const WienerPath& path,
                                                     double theta, const char* op) {
  if (!f) {
    ::pluss::throw_error(std::string("subsp::math::sde::") + op + ": integrand callback is required");
  }
  validate_path_(path, op);
  // The Series integral is vectorized over the time axis; adapt the per-step callback to it.
  const Series::CalcAccessor::IntegrandFn vectorized = [&](const Series& t, const Series& w) {
    const std::int64_t n = w.size(0);
    const std::vector<double> times = t.reshape({n}).to(pluss::pCPU, ScalarType::Float64).host<double>();
    Series stacked = pluss::series::undefined();
    for (std::int64_t i = 0; i < n; ++i) {
      const Series row = f(times[static_cast<std::size_t>(i)], w.select(0, i)).unsqueeze(0);
      stacked = stacked.defined() ? stacked.unite({row}, 0) : row;
    }
    return stacked;
  };
  const Series times = pluss::make_series(path.times, {static_cast<std::int64_t>(path.times.size())})
                           .to(path.w.scalar_type())
                           .to(path.w.platform());
  StochasticIntegralResult result;
  result.value = path.w.calc.antidiff.theta(vectorized, theta, times, 0);
  result.realized_quadratic_variation = quadratic_variation_(path);
  annotate_(result, path);
  return result;
}

}  // namespace detail

// Discrete Ito sum: sum_i f(t_i, W_{t_i}) * (W_{t_{i+1}} - W_{t_i}) --
// LEFT-endpoint evaluation, the defining, non-negotiable feature of the
// Ito convention (the integrand must not anticipate the increment it is
// about to be multiplied by). See the module comment for why this is not
// interchangeable with the Stratonovich convention below. The sum itself is
// `path.w.calc.antidiff.ito(...)` on Series; this function validates the path
// first and also reports the realized quadratic variation.
inline StochasticIntegralResult ito_integral(const WienerIntegrandFn& f, const WienerPath& path) {
  return detail::stochastic_integral_(f, path, 0.0, "ito_integral");
}

// Discrete Stratonovich sum: sum_i 0.5*(f(t_i, W_{t_i}) + f(t_{i+1},
// W_{t_{i+1}})) * (W_{t_{i+1}} - W_{t_i}) -- trapezoidal-averaged
// evaluation. Unlike Ito's, this convention obeys ordinary calculus's
// chain rule, which is exactly why it disagrees with ito_integral() by a
// term that is NOT an approximation error (see the test file).
inline StochasticIntegralResult strat_integral(const WienerIntegrandFn& f, const WienerPath& path) {
  return detail::stochastic_integral_(f, path, 0.5, "strat_integral");
}

// ---------------------------------------------------------------------
// Integrands that are computed from the whole sampled path.
//
// A pointwise callback f(t, W_t) cannot look ahead: it never sees a later sample. An integrand built from the path
// (a running maximum, a moving average, a filter) can, by accident -- a centered moving average or a "smoothed"
// value at sample i that uses sample i + 1 is the classic way to turn an Ito sum into something else without any
// error. The two overloads below take such an integrand as a function of the path and PROBE it before integrating:
// they keep the first j + 1 samples of the path, redraw the rest, and require the integrand at samples 0..j to be
// unchanged. An adapted integrand passes by construction; one that depends on the future fails with the first
// offending sample. The probe is randomized and finite (a dependence that happens to cancel for every redraw is not
// seen), so raise `probes` for an integrand you are unsure of.
// ---------------------------------------------------------------------
using SampledIntegrandFn = std::function<Series(const Series&)>;

struct AdaptednessOptions {
  std::int64_t probes{4};
  std::uint64_t seed{12345};
  double tolerance{1e-9};
};

namespace detail {

inline void check_adapted_(const SampledIntegrandFn& make, const WienerPath& path, const AdaptednessOptions& check,
                           const char* op) {
  const std::string name = std::string("subsp::math::sde::") + op;
  const std::int64_t n = path.w.size(0);
  const Series base = make(path.w);
  if (!base.defined() || base.dim() != path.w.dim() || (base.size(0) != n && base.size(0) != n - 1)) {
    ::pluss::throw_error(name + ": the integrand function must return one sample per path sample (or one fewer for "
                                "the Ito form), with the path's trailing shape");
  }
  for (std::int64_t d = 1; d < path.w.dim(); ++d) {
    if (base.size(d) != path.w.size(d)) {
      ::pluss::throw_error(name + ": the integrand's trailing shape must match the path's");
    }
  }
  if (!base.isfinite().all().to(pluss::pCPU, ScalarType::Bool).item<bool>()) {
    ::pluss::throw_error(name + ": the integrand contains a non-finite value");
  }
  if (n < 3 || check.probes <= 0) return;
  const double scale = 1.0 + base.abs().max().item<double>();
  const std::int64_t usable = base.size(0);
  for (std::int64_t k = 0; k < check.probes; ++k) {
    // split so that samples 0..j are kept and j + 1..n - 1 are redrawn
    const std::int64_t j = std::clamp<std::int64_t>(((k + 1) * (n - 1)) / (check.probes + 1), 1, n - 2);
    const std::int64_t tail = n - 1 - j;
    std::vector<double> root_dt(static_cast<std::size_t>(tail));
    for (std::int64_t i = 0; i < tail; ++i) {
      root_dt[static_cast<std::size_t>(i)] =
          std::sqrt(path.times[static_cast<std::size_t>(j + 1 + i)] - path.times[static_cast<std::size_t>(j + i)]);
    }
    std::vector<std::int64_t> bshape(static_cast<std::size_t>(path.w.dim()), 1);
    bshape[0] = tail;
    const Series scale_dt = pluss::make_series(root_dt, {tail}).to(path.w.scalar_type()).to(path.w.platform()).reshape(bshape);
    std::vector<std::int64_t> shape = {tail};
    for (std::int64_t d = 1; d < path.w.dim(); ++d) shape.push_back(path.w.size(d));
    const Series fresh = pluss::series::randn(pluss::msharp::RangeView::Int(shape),
                                              Config{}.with_dtype(path.w.scalar_type()).with_platform(path.w.platform()),
                                              check.seed + static_cast<std::uint64_t>(k)).mul(scale_dt);
    const Series kept = path.w.slice(0, 0, j + 1, 1);
    const Series redrawn = kept.select(0, j).unsqueeze(0).add(fresh.cumsum(0));
    const Series perturbed = kept.unite({redrawn}, 0);
    const Series changed = make(perturbed);
    if (!changed.defined() || changed.size(0) != usable) {
      ::pluss::throw_error(name + ": the integrand function changed its length when the path changed");
    }
    const std::int64_t compared = std::min<std::int64_t>(j + 1, usable);
    const Series diff = base.slice(0, 0, compared, 1).sub(changed.slice(0, 0, compared, 1)).abs();
    Series per_sample = diff;
    while (per_sample.dim() > 1) per_sample = per_sample.amax(per_sample.dim() - 1, false);
    const std::vector<double> worst = per_sample.to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::int64_t i = 0; i < compared; ++i) {
      if (worst[static_cast<std::size_t>(i)] > check.tolerance * scale) {
        ::pluss::throw_error(name + ": the integrand at sample " + std::to_string(i) +
                             " changed when only samples after sample " + std::to_string(j) +
                             " of the path were redrawn, so it looks into the future of the path (it is not "
                             "adapted) and the sum is not an Ito integral");
      }
    }
  }
}

inline StochasticIntegralResult sampled_integral_(const SampledIntegrandFn& make, const WienerPath& path,
                                                  const AdaptednessOptions& check, double theta, const char* op) {
  if (!make) {
    ::pluss::throw_error(std::string("subsp::math::sde::") + op + ": integrand function is required");
  }
  validate_path_(path, op);
  check_adapted_(make, path, check, op);
  StochasticIntegralResult result;
  result.value = make(path.w).antidiff(path.w).theta(theta, 0);
  result.realized_quadratic_variation = quadratic_variation_(path);
  annotate_(result, path);
  return result;
}

}  // namespace detail

// Ito sum of an integrand computed from the sampled path, after checking that it is adapted.
inline StochasticIntegralResult ito_integral(const SampledIntegrandFn& make, const WienerPath& path,
                                             const AdaptednessOptions& check = {}) {
  return detail::sampled_integral_(make, path, check, 0.0, "ito_integral");
}

inline StochasticIntegralResult strat_integral(const SampledIntegrandFn& make, const WienerPath& path,
                                               const AdaptednessOptions& check = {}) {
  return detail::sampled_integral_(make, path, check, 0.5, "strat_integral");
}

// ---------------------------------------------------------------------
// Jump paths and the Marcus (canonical) integral
// ---------------------------------------------------------------------

// A path with Brownian increments and compound-Poisson jumps (Merton's jump-diffusion driver),
// X = drift * t + volatility * W + sum of jumps, together with the steps that contain a jump.
struct JumpPath {
  WienerPath path;
  Series jump_mask = pluss::series::undefined();  // [steps] + like: 1 on a step with a jump
};

// At most one jump per step is simulated, so jump_rate * h must be small (checked: below 0.2).
inline JumpPath simulate_jump_diffusion_path(const Series& like, double t0, double t1, std::int64_t steps,
                                             std::uint64_t seed, double drift, double volatility, double jump_rate,
                                             double jump_mean, double jump_stddev) {
  if (steps <= 0) ::pluss::throw_error("subsp::math::sde::simulate_jump_diffusion_path: steps must be > 0");
  if (!(jump_rate >= 0.0) || !(jump_stddev >= 0.0) || !(volatility >= 0.0)) {
    ::pluss::throw_error("subsp::math::sde::simulate_jump_diffusion_path: rate, volatilities must be non-negative");
  }
  const double h = (t1 - t0) / static_cast<double>(steps);
  if (jump_rate * h >= 0.2) {
    ::pluss::throw_error("subsp::math::sde::simulate_jump_diffusion_path: jump_rate * step is " +
                         std::to_string(jump_rate * h) + "; use more steps (at most one jump per step is simulated)");
  }
  std::vector<std::int64_t> sizes = {steps};
  for (std::int64_t d = 0; d < like.dim(); ++d) sizes.push_back(like.size(d));
  const auto cfg = Config{}.with_dtype(like.scalar_type()).with_platform(like.platform());
  const Series brownian = pluss::series::randn(pluss::msharp::RangeView::Int(sizes), cfg, seed).mul(volatility * std::sqrt(h));
  const Series u = Series::rand(pluss::msharp::RangeView::Int(sizes), like.scalar_type(), seed + 1, like.platform());
  const Series size = pluss::series::randn(pluss::msharp::RangeView::Int(sizes), cfg, seed + 2).mul(jump_stddev).add(jump_mean);
  const Series mask = u.lt(jump_rate * h).to(like.scalar_type());
  const Series increments = brownian.add(drift * h).add(mask.mul(size));
  const Series zero = Series::zeros(like.sizes(), like.scalar_type(), like.platform()).unsqueeze(0);
  JumpPath out;
  out.path.w = zero.unite({increments.cumsum(0)}, 0);
  out.path.t0 = t0;
  out.path.t1 = t1;
  out.path.steps = steps;
  for (std::int64_t i = 0; i <= steps; ++i) out.path.times.push_back(t0 + h * static_cast<double>(i));
  out.jump_mask = mask;
  return out;
}

// Marcus integral of f(t, X_t) dX along a path with jumps. Steps flagged in `jump_mask` are integrated along the
// straight segment between the samples (the flow of the jump); the rest use the Stratonovich average. With the
// second overload, a step is flagged when |dX| > jump_sigma * sqrt(step), i.e. far outside the Brownian scale --
// pass an explicit mask when you know where the jumps are.
inline StochasticIntegralResult marcus_integral(const WienerIntegrandFn& f, const WienerPath& path,
                                                const Series& jump_mask) {
  if (!f) ::pluss::throw_error("subsp::math::sde::marcus_integral: integrand callback is required");
  detail::validate_path_(path, "marcus_integral");
  if (!jump_mask.defined() || jump_mask.size(0) != path.steps) {
    ::pluss::throw_error("subsp::math::sde::marcus_integral: the jump mask needs one entry per step");
  }
  const Series::CalcAccessor::IntegrandFn vectorized = [&](const Series& t, const Series& w) {
    const std::int64_t n = w.size(0);
    const std::vector<double> times = t.reshape({n}).to(pluss::pCPU, ScalarType::Float64).host<double>();
    Series stacked = pluss::series::undefined();
    for (std::int64_t i = 0; i < n; ++i) {
      const Series row = f(times[static_cast<std::size_t>(i)], w.select(0, i)).unsqueeze(0);
      stacked = stacked.defined() ? stacked.unite({row}, 0) : row;
    }
    return stacked;
  };
  const Series times = pluss::make_series(path.times, {static_cast<std::int64_t>(path.times.size())})
                           .to(path.w.scalar_type())
                           .to(path.w.platform());
  StochasticIntegralResult result;
  result.value = path.w.calc.antidiff.marcus(vectorized, jump_mask, times, 0);
  result.realized_quadratic_variation = detail::quadratic_variation_(path);
  detail::annotate_(result, path);
  return result;
}

inline StochasticIntegralResult marcus_integral(const WienerIntegrandFn& f, const WienerPath& path,
                                                double jump_sigma = 8.0) {
  detail::validate_path_(path, "marcus_integral");
  if (!(jump_sigma > 0.0)) ::pluss::throw_error("subsp::math::sde::marcus_integral: jump_sigma must be positive");
  std::vector<double> bound(static_cast<std::size_t>(path.steps));
  for (std::int64_t i = 0; i < path.steps; ++i) {
    bound[static_cast<std::size_t>(i)] =
        jump_sigma * std::sqrt(path.times[static_cast<std::size_t>(i + 1)] - path.times[static_cast<std::size_t>(i)]);
  }
  std::vector<std::int64_t> bshape(static_cast<std::size_t>(path.w.dim()), 1);
  bshape[0] = path.steps;
  const Series limit = pluss::make_series(bound, {path.steps}).to(path.w.scalar_type()).to(path.w.platform()).reshape(bshape);
  const Series mask = path.w.diff(1, 0).abs().gt(limit);
  return marcus_integral(f, path, mask.to(path.w.scalar_type()));
}

}  // namespace pluss::subsp::math::sde
