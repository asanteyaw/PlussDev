#pragma once

// pluss::subsp::math::integ -- Series-native numerical integration.
//
// Naming: this module's algorithm family follows the exact same convention
// PlussMa's optimization module already established (Septima, Optima,
// Minimax -- Latin feminine-adjective names, not generic technical labels).
// The names here:
//
//   Integra    -- "whole, complete, intact" (Latin) -- the literal
//                 etymological root of "integral" itself. The top-level
//                 convenience dispatcher, mirroring how septima()/optima()
//                 are the friendly top-level solve entry points sitting
//                 above their own lower-level building blocks.
//   Quadra     -- "a square/quarter" (Latin), root of "quadrature". The
//                 deterministic engine: composite Trapezoidal, Simpson, and
//                 panelled Gauss-Legendre rules, plus the sampled-data
//                 trapezoid()/cumulative_trapezoid() pair (torch parity).
//   Aleatoria  -- "of chance/dice" (Latin, root of "aleatory"). The Monte
//                 Carlo engine: vectorized uniform (optionally stratified)
//                 sampling, 1-D and N-D.
//   Analytica  -- "analytical/exact". A small, explicitly-invoked, growable
//                 registry of closed-form solutions for integrand families
//                 the rest of the codebase already needs (Gaussian,
//                 polynomial, exponential, power-law).
//
// Design philosophy, borrowed from diff.h but adapted to what integration
// actually needs:
//
//   - diff.h's split is forward-mode (DualTrace, tangent propagation) vs.
//     reverse-mode (Trace, tape-based pullback) -- two fundamentally
//     different COMPUTATIONAL modes for the same operation (differentiate).
//     Integration has no such duality; instead it has genuinely different
//     ALGORITHMIC STRATEGIES (deterministic quadrature vs. Monte Carlo vs.
//     closed-form), so that is the split used here (Quadra/Aleatoria/
//     Analytica) rather than forcing an artificial forward/reverse-style
//     division that doesn't correspond to anything real for this problem.
//   - Free functions returning an Options/Result struct pair, not classes
//     -- exactly diff.h's (jvp/vjp/hvp) and optim.h's (septima/
//     l_sr1_trust_region) own shape. A *Result struct (value + diagnostics:
//     error estimate, evaluation count, convergence flag) mirrors
//     SeptimaResult/Lsr1Result's own field conventions.
//   - The single highest-leverage performance decision: IntegrandFn is
//     `Series -> Series` (a whole batch of quadrature nodes or Monte Carlo
//     samples in, one elementwise-evaluated batch out), NEVER a per-point
//     scalar Trace/double callback evaluated in a C++ loop. Every engine
//     below builds ONE flat node/sample array and calls the integrand
//     EXACTLY ONCE per resolution level. This means "highly optimized for
//     perf" falls out for free: the integrand's own body is ordinary
//     Series arithmetic, so it already runs through CodeM's existing
//     native/BLAS/MPS/CUDA-dispatched vectorized kernels -- this module
//     does no elementwise math of its own beyond weighting and reducing
//     the batch CodeM already computed efficiently.
//   - Differentiability comes for free, with no Trace plumbing needed
//     anywhere in this file: trapezoid/cumulative_trapezoid/quadra/
//     aleatoria are built ENTIRELY from ordinary autodiff-tracked Series
//     primitives (slice, diff, mul, add, sum, cumsum, mean). If an
//     integrand closure captures a Series that is itself part of an
//     active reverse-mode tape (e.g. a distribution parameter being
//     learned), the resulting integral is automatically part of that same
//     tape and differentiates correctly through .reverse() -- exactly the
//     "compose from already-differentiable primitives, inherit the
//     property for free" pattern used throughout this codebase (Series::
//     abs2() = abs().square(), Minimax's pure-add/mul equality-multiplier
//     update, etc.). A caller working inside a Trace-based objective can
//     freely mix in a call to this module: extract `.value()` to get the
//     plain Series, integrate, and the result is already a valid,
//     tape-linked Series usable anywhere a Trace-derived value is (or
//     wrap it back with `Trace(result)` if a literal Trace is wanted).
//   - Analytica is deliberately NOT part of Integra's automatic strategy
//     selection. Recognizing that an arbitrary `Series(const Series&)`
//     callback matches a known closed form is a symbolic pattern-matching
//     problem -- effectively a small computer-algebra system -- and that
//     is out of scope here. Analytica instead exposes a small set of
//     directly-invoked, exactly-named closed-form functions (gaussian,
//     polynomial, exponential, power) for integrand FAMILIES the rest of
//     the codebase already needs; callers consult it explicitly when they
//     already know their integrand matches one of these forms, and fall
//     back to Quadra/Aleatoria otherwise. Growable, never "automatic".
//
// This module also replaces THREE near-identical private composite-Simpson
// helpers found reimplementing the identical fixed-node formula across the
// tree with one shared, tested implementation (`quadra_scalar`, below) --
// the scalar/legacy counterpart to `quadra` for callers whose integrand is
// genuinely a plain `double(double)` function outside the Series/autodiff
// machinery entirely, not something that benefits from batched evaluation:
// `integrate_simpson_scalar_` in PlussMa/Stats/distributions/distr.cpp,
// `integrate_simpson` in distr_objects_continuous.cpp, and
// `simpson_integral_` in PlussMa/Math/Finance/pricing/heston_sv.cpp (the
// characteristic-function inversion integral behind Heston/Bates option
// pricing). Deliberately NOT touched: heston_sv.cpp's separate Carr-Madan
// FFT-grid Simpson/Trapezoidal WEIGHTING scheme (`HestonFftQuadrature`) --
// that produces an entire strike grid from one FFT/FRFT transform, not a
// direct numerical integral, and isn't expressible as a quadra()/
// quadra_scalar() call without restructuring the pricing algorithm itself;
// left alone on purpose, not an oversight.
//
// Verified capabilities, each with real test coverage (not just asserted):
//  - Autograd: trapezoid()/cumulative_trapezoid()/quadra()/aleatoria() all
//    have a `.reverse()`-based gradient test against a hand-derived closed
//    form, not just a forward-value check.
//  - Complex integrands: trapezoid()/quadra()/aleatoria() are each
//    verified with a genuinely complex-valued (not just real-cast)
//    integrand, including a characteristic-function-shaped one
//    (exp(-x)*exp(i*x)) over a semi-infinite domain.
//  - Semi-infinite/infinite domains: quadra()/aleatoria() detect +-
//    infinity bounds and transparently apply the standard change-of-
//    variables substitution (see the substitution helpers below) --
//    exactly what a genuine characteristic-function/Fourier-inversion
//    integral needs, as opposed to the large-finite-cutoff truncation
//    heston_sv.cpp currently uses instead. Migrating those call sites onto
//    genuine infinite-domain integration (instead of the manual
//    `integration_u_max` truncation they use today) is a real, separate,
//    deliberately-not-yet-made decision: it would change Heston/Bates
//    pricing's actual numeric output, however slightly, which needs its
//    own explicit sign-off given the financial-correctness stakes, not a
//    silent change bundled into a library refactor.
//  - Batched quadra(): `fn` may return any number of leading batch axes
//    ([...,N] instead of just [N]) as long as the node axis is last --
//    every rule reduces along `y.dim()-1` specifically, never a hardcoded
//    axis, so one quadra() call can price a whole batch of independent
//    integrals (e.g. a strike/maturity grid) sharing the same quadrature
//    nodes in one vectorized pass, gradients included.
//
// Explicitly NOT built, and out of scope for this module: ODE/IVP
// integration (Runge-Kutta or otherwise) and adjoint-sensitivity
// backpropagation through an ODE solve. Nothing like it exists anywhere
// in this repo. It would be new work from scratch and (per the review
// that prompted this list) is the one place a genuinely separate
// backward-pass design -- the adjoint method, as in torchdiffeq/diffrax --
// actually earns its keep, unlike everywhere else in this file, which
// differentiates for free by composing already-differentiable Series ops.

#include "pluss/subsp/math/diff.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::math::integ {

// A batched, Series-native integrand: given a Series of quadrature nodes or
// Monte Carlo sample points (any shape), returns the integrand evaluated at
// every one of them in the SAME shape (elementwise) -- ordinary Series
// arithmetic, so it runs through CodeM's own vectorized kernels and is
// autodiff-differentiable for free. This is the primary integrand type
// used throughout this module.
using IntegrandFn = std::function<Series(const Series&)>;

// N-dimensional counterpart: one Series of batched sample coordinates per
// axis (all the same length), used by aleatoria()'s multi-dimensional
// overload.
using IntegrandNdFn = std::function<Series(const std::vector<Series>&)>;

// A plain scalar integrand, for callers whose function is genuinely outside
// the Series/autodiff machinery (e.g. a CDF's internal PDF evaluated at one
// point via ordinary double arithmetic) -- see quadra_scalar below.
using ScalarIntegrandFn = std::function<double(double)>;

namespace detail {

inline void require_defined_(const Series& x, const char* op) {
  if (!x.defined()) {
    ::pluss::throw_error(std::string("subsp::math::integ::") + op + ": expected defined Series");
  }
}

inline bool is_complex_(const Series& x) {
  return x.defined() && x.dtype().is_complex();
}

// Reduces a (possibly complex, possibly BATCHED) Series to a single real
// double for convergence checks / error estimates: .abs() maps a real
// value to itself and a complex value to its magnitude (dtype-agnostic),
// then .max() collapses to one worst-case scalar regardless of shape -- a
// no-op for an already-scalar Series (every call site before batched
// quadra() support was added), and the conservative choice for a batched
// one (every row must have converged, not just the average).
inline double scalar_magnitude_(const Series& x) {
  return x.abs().max().to(pluss::pCPU, ScalarType::Float64).item<double>();
}

}  // namespace detail

// ---------------------------------------------------------------------
// Sampled-data integration (torch parity): trapezoid()/cumulative_
// trapezoid(). The canonical implementation lives as a genuine `Series`
// method (PlussQit -- see Series::trapezoid/Series::cumulative_trapezoid
// in CodeM/core/Series.h + OpsInternal.cpp), not here: PlussMa depends on
// PlussQit, never the other way around, so PlussQit's own `Series` class
// cannot call into this PlussMa module -- these free functions exist
// purely as this module's own namespaced surface (`pluss::integ::
// trapezoid(y, x)`, matching every other engine in this file's calling
// convention) and simply forward to `y`'s own `antidiff` ACCESSOR
// (`Series::AntidiffAccessor`/`SeriesAntidiffProxy` in
// CodeM/core/Series.h -- `y.antidiff().trapezoid(x)` /
// `y.antidiff.trapezoid(x)`), not a flat Series method: this mirrors the
// SAME dual accessor pattern Series already uses for `.stat()`/
// `.linearalg()` (a nested Proxy class doing the real work, plus a
// dot-style-convenience Accessor field forwarding to it), rather than
// adding more names directly to Series's own flat method surface. It is
// named `antidiff` (not `integ`) because it reverses `Series::diff()`
// (discrete forward differencing on already-materialized data) -- this
// deliberately does NOT extend to the continuous/callable-integrand
// quadrature below (`quadra`'s 1-D/N-D overloads, `aleatoria`,
// `Analytica`), which stays under the `integ` namespace: those need to
// re-invoke a function at points they choose, which a single
// already-computed Series value cannot provide (see the module-level
// comment near their definitions).
// Both call paths (`pluss::integ::trapezoid(y, x)` and
// `y.antidiff().trapezoid(x)`) run the exact same code -- there is no
// duplicate formula to keep in sync. Built entirely from existing Series
// primitives (diff, slice, mul, add, sum/cumsum): no new CodeM kernel, no
// backward-manifest entry needed, differentiable for free, and correct
// for complex y with no extra work (these formulas are pure linear
// combination -- no ordering/isfinite-style operation appears anywhere in
// them, unlike the optimizer-layer complex fixes elsewhere in this
// codebase). Matches torch.trapezoid/torch.cumulative_trapezoid's own
// calling convention too: real torch's native_functions.yaml declares
// these with no `variants:` line (defaults to function-only, no Tensor
// method in real torch) and no `dispatch:` entry either, meaning torch
// itself implements them compositionally from other primitives rather
// than a dedicated kernel -- the same approach taken here.
// ---------------------------------------------------------------------

inline Series trapezoid(const Series& y, const Series& x, std::int64_t dim = -1) {
  return y.antidiff().trapezoid(x, dim);
}
inline Series trapezoid(const Series& y, double dx, std::int64_t dim = -1) {
  return y.antidiff().trapezoid(dx, dim);
}
inline Series trapezoid(const Series& y, std::int64_t dim = -1) { return y.antidiff().trapezoid(dim); }

inline Series cumulative_trapezoid(const Series& y, const Series& x, std::int64_t dim = -1) {
  return y.antidiff().cumulative_trapezoid(x, dim);
}
inline Series cumulative_trapezoid(const Series& y, double dx, std::int64_t dim = -1) {
  return y.antidiff().cumulative_trapezoid(dx, dim);
}
inline Series cumulative_trapezoid(const Series& y, std::int64_t dim = -1) {
  return y.antidiff().cumulative_trapezoid(dim);
}

// ---------------------------------------------------------------------
// Quadra: deterministic quadrature of a callable integrand over [a, b].
// ---------------------------------------------------------------------

enum class QuadraRule : std::uint8_t {
  Trapezoidal = 0,
  Simpson = 1,
  GaussLegendre = 2,
  // Tries GaussLegendre first (spectral accuracy, the best default for a
  // generic smooth integrand), and escalates to Simpson then Trapezoidal
  // ONLY if the previous rule genuinely fails to converge within its own
  // adaptive refinement budget (options.max_refinements at options.
  // tolerance) -- a real, runtime-OBSERVED escalation trigger (did it
  // converge?), not an attempt to inspect fn's structure. That inspection
  // isn't possible in general: fn is an opaque callable (see the
  // IntegrandFn comment above), so there is no way to know ahead of time
  // "this integrand needs Trapezoidal" the way a symbolic system could --
  // the only signal available is whether a rule's own step-doubling
  // refinement is actually shrinking the error estimate.
  Auto = 3,
};

struct QuadraOptions {
  QuadraRule rule{QuadraRule::Simpson};
  std::int64_t nodes{257};        // Trapezoidal/Simpson: total node count (Simpson rounds up to odd).
  std::int64_t panels{32};        // GaussLegendre: number of composite panels.
  std::int64_t gauss_points{5};   // GaussLegendre: 5 or 10 (fixed-order tabulated rules).
  bool adaptive{true};            // Step-double the resolution until error_estimate <= tolerance.
  double tolerance{1e-10};
  std::int64_t max_refinements{10};
  ScalarType dtype{ScalarType::Float64};
  pluss::codem::Platform platform{pluss::codem::Platform::CPU};
  // Only used when `a`/`b` passed to quadra() is +-infinity: how close to
  // the substituted domain's open endpoint (where the change-of-variables
  // Jacobian is singular) to actually integrate -- see the substitution
  // helpers above. Smaller reaches further into the tail at the cost of a
  // more extreme Jacobian value near the clipped edge.
  double infinite_domain_epsilon{1e-10};
};

struct QuadraResult {
  Series value = pluss::series::undefined();
  double error_estimate{std::numeric_limits<double>::infinity()};
  std::int64_t evaluations{0};
  std::int64_t refinements{0};
  bool converged{false};
  // Which concrete rule actually produced `value` -- always equal to
  // options.rule unless options.rule was Auto, in which case this reports
  // where the escalation ladder stopped (see QuadraRule::Auto).
  QuadraRule rule_used{QuadraRule::Trapezoidal};
};

namespace detail {

// `y = fn(x)` may be the plain [n+1] elementwise evaluation (the common
// case), or a BATCHED [...,n+1] evaluation -- any number of leading batch
// axes, as long as the node axis is last and every rule below reduces
// specifically along `y.dim()-1`, never a hardcoded axis 0. This is what
// lets one quadra() call price an entire batch of independent integrals
// (e.g. a whole strike/maturity grid) sharing the same quadrature nodes,
// in one vectorized pass, instead of one call per point -- the caller's
// own `fn` is responsible for producing that batched shape (typically by
// broadcasting `x` against its own per-row parameters), nothing here
// needs to know how many batch rows there are.
inline Series quadra_trapezoidal_(const IntegrandFn& fn, double a, double b, std::int64_t n, ScalarType dtype,
                                  pluss::codem::Platform platform, std::int64_t* evaluations) {
  const Series x = Series::linspace(a, b, n + 1, dtype, platform);
  const Series y = fn(x);
  require_defined_(y, "quadra");
  if (evaluations) *evaluations += n + 1;
  return trapezoid(y, x, y.dim() - 1);
}

inline Series quadra_simpson_(const IntegrandFn& fn, double a, double b, std::int64_t n, ScalarType dtype,
                              pluss::codem::Platform platform, std::int64_t* evaluations) {
  if (n < 2) n = 2;
  if (n % 2 != 0) ++n;
  const Series x = Series::linspace(a, b, n + 1, dtype, platform);
  const Series y = fn(x);
  require_defined_(y, "quadra");
  if (evaluations) *evaluations += n + 1;
  const double h = (b - a) / static_cast<double>(n);
  std::vector<double> w(static_cast<std::size_t>(n + 1));
  w.front() = 1.0;
  w.back() = 1.0;
  for (std::int64_t i = 1; i < n; ++i) {
    w[static_cast<std::size_t>(i)] = (i % 2 == 0) ? 2.0 : 4.0;
  }
  // `weights` is always 1-D [n+1]; ordinary elementwise broadcasting
  // aligns it against y's trailing (node) axis regardless of how many
  // leading batch axes y has, so no explicit reshape is needed here the
  // way trapezoid()'s own x-broadcast needed one (x itself is a genuine
  // coordinate that trapezoid()'s formula indexes into per-element, not a
  // plain elementwise multiplicand).
  const Series weights = pluss::make_series(w, {n + 1}).to(dtype).to(platform);
  return y.mul(weights).sum(y.dim() - 1).mul(h / 3.0);
}

struct GaussLegendreTable {
  std::vector<double> nodes;    // on [-1, 1]
  std::vector<double> weights;  // sum to 2.0
};

// Fixed-order tabulated Gauss-Legendre nodes/weights (standard reference
// values; e.g. Abramowitz & Stegun table 25.4, or any numerical-methods
// reference -- these are universal constants, not derived here).
inline const GaussLegendreTable& gauss_legendre_table_(std::int64_t points) {
  static const GaussLegendreTable table5{
      {-0.9061798459386640, -0.5384693101056831, 0.0, 0.5384693101056831, 0.9061798459386640},
      {0.2369268850561891, 0.4786286704993665, 0.5688888888888889, 0.4786286704993665, 0.2369268850561891}};
  static const GaussLegendreTable table10{
      {-0.9739065285171717, -0.8650633666889845, -0.6794095682990244, -0.4333953941292472, -0.1488743389816312,
       0.1488743389816312, 0.4333953941292472, 0.6794095682990244, 0.8650633666889845, 0.9739065285171717},
      {0.0666713443086881, 0.1494513491505806, 0.2190863625159820, 0.2692667193099963, 0.2955242247147529,
       0.2955242247147529, 0.2692667193099963, 0.2190863625159820, 0.1494513491505806, 0.0666713443086881}};
  if (points == 10) return table10;
  return table5;
}

inline Series quadra_gauss_legendre_(const IntegrandFn& fn, double a, double b, std::int64_t panels,
                                     std::int64_t points, ScalarType dtype, pluss::codem::Platform platform,
                                     std::int64_t* evaluations) {
  if (panels < 1) panels = 1;
  const GaussLegendreTable& table = gauss_legendre_table_(points);
  const std::size_t per_panel = table.nodes.size();
  const double panel_width = (b - a) / static_cast<double>(panels);

  // Every panel's nodes/weights are built into ONE flat array up front, so
  // fn() is still called exactly once (batched) regardless of panel count
  // -- the same "one call per resolution level" discipline as the other
  // rules, not a per-panel loop calling into Series arithmetic repeatedly.
  std::vector<double> all_x(static_cast<std::size_t>(panels) * per_panel);
  std::vector<double> all_w(static_cast<std::size_t>(panels) * per_panel);
  for (std::int64_t p = 0; p < panels; ++p) {
    const double lo = a + static_cast<double>(p) * panel_width;
    const double mid = lo + panel_width * 0.5;
    const double half = panel_width * 0.5;
    for (std::size_t k = 0; k < per_panel; ++k) {
      const std::size_t idx = static_cast<std::size_t>(p) * per_panel + k;
      all_x[idx] = mid + half * table.nodes[k];
      all_w[idx] = half * table.weights[k];
    }
  }
  const auto count = static_cast<std::int64_t>(all_x.size());
  const Series x = pluss::make_series(all_x, {count}).to(dtype).to(platform);
  const Series w = pluss::make_series(all_w, {count}).to(dtype).to(platform);
  const Series y = fn(x);
  require_defined_(y, "quadra");
  if (evaluations) *evaluations += count;
  return y.mul(w).sum(y.dim() - 1);
}

// ---------------------------------------------------------------------
// Semi-infinite/infinite domain support, via the standard change-of-
// variables substitutions (the same technique real torch/JAX quadrature
// libraries use, and the concrete thing needed for Fourier/characteristic-
// function-style integrands over [0, infinity) or (-infinity, infinity) --
// PlussMa/Finance's Heston/Bates characteristic-function inversion
// integrals currently sidestep this need entirely by manually truncating
// to a large finite cutoff (`options.integration_u_max`) rather than
// integrating the true infinite domain; these substitutions are what would
// let that be a real infinite integral instead, though migrating those
// call sites is a separate, deliberately not-yet-made decision -- see the
// module-level test file for the discussion).
//
//   x = a + t/(1-t),  t in [0,1)        -> x in [a, +infinity)
//   x = b - t/(1-t),  t in [0,1)        -> x in (-infinity, b]
//   x = t/(1-t^2),    t in (-1,1)       -> x in (-infinity, +infinity)
//
// Each rewrites the ORIGINAL integrand into a new one (over the finite
// substituted domain) that already folds in the Jacobian |dx/dt| -- so the
// caller-facing quadra()/aleatoria() simply recurse into their own
// existing finite-domain implementation with the rewritten integrand and
// domain, reusing every bit of machinery (rule dispatch, adaptive
// refinement, the complex-magnitude convergence check, uniform sampling)
// rather than duplicating it. The substituted domain's open endpoint
// (t -> 1, where the Jacobian blows up) is clipped by a small epsilon
// rather than integrated exactly -- a real, standard, and clearly
// documented truncation (not integrating literally all the way to
// infinity, but close enough that the omitted tail contributes below
// floating-point precision for any integrand that actually decays, which
// characteristic functions do).
inline Series substitution_jacobian_upper_(const Series& one_minus_t) { return one_minus_t.pow(-2.0); }

inline IntegrandFn substitute_upper_infinite_(const IntegrandFn& fn, double a) {
  return [fn, a](const Series& t) -> Series {
    const Series one_minus_t = t.mul(-1.0).add(1.0);
    const Series x = t.div(one_minus_t).add(a);
    return fn(x).mul(substitution_jacobian_upper_(one_minus_t));
  };
}

inline IntegrandFn substitute_lower_infinite_(const IntegrandFn& fn, double b) {
  return [fn, b](const Series& t) -> Series {
    const Series one_minus_t = t.mul(-1.0).add(1.0);
    const Series x = t.div(one_minus_t).mul(-1.0).add(b);
    return fn(x).mul(substitution_jacobian_upper_(one_minus_t));
  };
}

inline IntegrandFn substitute_both_infinite_(const IntegrandFn& fn) {
  return [fn](const Series& t) -> Series {
    const Series t2 = t.mul(t);
    const Series one_minus_t2 = t2.mul(-1.0).add(1.0);
    const Series x = t.div(one_minus_t2);
    const Series jac = t2.add(1.0).div(one_minus_t2.mul(one_minus_t2));
    return fn(x).mul(jac);
  };
}

// Runs exactly ONE concrete rule (never Auto -- the caller resolves Auto
// into a sequence of calls to this, see quadra()'s own Auto branch) over
// an already-finite [lo, hi], with its own adaptive step-doubling
// refinement. Factored out of quadra() so the Auto escalation ladder can
// call it once per rung without duplicating the refinement loop.
inline QuadraResult quadra_single_rule_(const IntegrandFn& fn, double lo, double hi, QuadraRule rule,
                                        const QuadraOptions& options) {
  QuadraResult result;
  result.rule_used = rule;

  auto eval_at_ = [&](std::int64_t resolution) -> Series {
    switch (rule) {
      case QuadraRule::Trapezoidal:
        return quadra_trapezoidal_(fn, lo, hi, resolution, options.dtype, options.platform, &result.evaluations);
      case QuadraRule::Simpson:
        return quadra_simpson_(fn, lo, hi, resolution, options.dtype, options.platform, &result.evaluations);
      case QuadraRule::GaussLegendre:
      case QuadraRule::Auto:
      default:
        return quadra_gauss_legendre_(fn, lo, hi, resolution, options.gauss_points, options.dtype, options.platform,
                                      &result.evaluations);
    }
  };

  std::int64_t resolution = (rule == QuadraRule::GaussLegendre) ? options.panels : options.nodes;
  Series estimate = eval_at_(resolution);

  if (options.adaptive) {
    double prev = scalar_magnitude_(estimate);
    for (std::int64_t r = 0; r < options.max_refinements; ++r) {
      resolution *= 2;
      const Series refined = eval_at_(resolution);
      const double refined_mag = scalar_magnitude_(refined);
      const double err = std::abs(refined_mag - prev);
      result.refinements = r + 1;
      result.error_estimate = err;
      estimate = refined;
      if (err <= options.tolerance * std::max(1.0, refined_mag)) {
        result.converged = true;
        break;
      }
      prev = refined_mag;
    }
  } else {
    result.converged = true;
  }

  result.value = estimate;
  return result;
}

}  // namespace detail

inline QuadraResult quadra(const IntegrandFn& fn, double a, double b, const QuadraOptions& options = {}) {
  if (!fn) {
    ::pluss::throw_error("subsp::math::integ::quadra: integrand callback is required");
  }
  if (!(options.tolerance >= 0.0) || !std::isfinite(options.tolerance)) {
    ::pluss::throw_error("subsp::math::integ::quadra: tolerance must be finite and >= 0");
  }
  if (options.max_refinements < 0) {
    ::pluss::throw_error("subsp::math::integ::quadra: max_refinements must be >= 0");
  }

  // Semi-infinite/infinite bounds: rewrite into an equivalent finite-domain
  // problem via the substitutions above, then recurse -- the recursive
  // call always has finite bounds, so this terminates in exactly one
  // extra call. See the substitution helpers' own comment for the
  // rationale and the epsilon-clipping tradeoff.
  const bool a_inf = std::isinf(a);
  const bool b_inf = std::isinf(b);
  if (a_inf || b_inf) {
    const double eps = options.infinite_domain_epsilon;
    if (a_inf && b_inf) {
      if (!(a < 0.0) || !(b > 0.0)) {
        ::pluss::throw_error("subsp::math::integ::quadra: expected a = -infinity, b = +infinity for a fully "
                             "infinite domain");
      }
      return quadra(detail::substitute_both_infinite_(fn), -1.0 + eps, 1.0 - eps, options);
    }
    if (b_inf) {
      if (!(b > 0.0)) {
        ::pluss::throw_error("subsp::math::integ::quadra: expected b = +infinity for a semi-infinite upper bound");
      }
      return quadra(detail::substitute_upper_infinite_(fn, a), 0.0, 1.0 - eps, options);
    }
    if (!(a < 0.0)) {
      ::pluss::throw_error("subsp::math::integ::quadra: expected a = -infinity for a semi-infinite lower bound");
    }
    return quadra(detail::substitute_lower_infinite_(fn, b), 0.0, 1.0 - eps, options);
  }

  QuadraResult result;
  if (a == b) {
    result.value = pluss::make_series(0.0, Config{}.with_dtype(options.dtype).with_platform(options.platform));
    result.converged = true;
    return result;
  }
  const bool reversed = b < a;
  const double lo = reversed ? b : a;
  const double hi = reversed ? a : b;

  if (options.rule == QuadraRule::Auto) {
    // Escalation ladder: GaussLegendre -> Simpson -> Trapezoidal, stopping
    // at the first rung that converges. `evaluations` accumulates across
    // every rung actually tried (an honest total of function calls spent,
    // including a rung that failed to converge), while
    // error_estimate/refinements/rule_used/converged/value all come from
    // whichever rung the loop stopped on.
    static constexpr QuadraRule kLadder[3] = {QuadraRule::GaussLegendre, QuadraRule::Simpson,
                                              QuadraRule::Trapezoidal};
    std::int64_t total_evaluations = 0;
    for (const QuadraRule rung : kLadder) {
      result = detail::quadra_single_rule_(fn, lo, hi, rung, options);
      total_evaluations += result.evaluations;
      result.evaluations = total_evaluations;
      if (result.converged) break;
    }
  } else {
    result = detail::quadra_single_rule_(fn, lo, hi, options.rule, options);
  }

  result.value = reversed ? result.value.neg() : result.value;
  return result;
}

// ---------------------------------------------------------------------
// quadra (N-D overload): multi-dimensional deterministic quadrature via a
// tensor-product rule -- the genuine N-D counterpart to the 1-D quadra()
// overload above, distinguished purely by parameter types (IntegrandNdFn +
// a vector of per-axis bounds, vs IntegrandFn + a single (a, b) pair) --
// same overload-on-name pattern aleatoria() already uses for its own 1-D/
// N-D split, rather than a separately-named quadra_nd() (as it used to
// be before the two were merged into one overloaded `quadra`). `fn` takes one
// coordinate Series per dimension (the same IntegrandNdFn aleatoria()'s
// N-D overload already uses), each of length equal to the TOTAL flattened
// grid size (the product of every axis's own node count) -- the standard
// "meshgrid, then flatten" construction. Every axis uses the SAME rule
// (Trapezoidal/Simpson/GaussLegendre) and the SAME per-axis resolution;
// per-axis-varying resolution is not exposed here (a straightforward
// extension if ever needed, not built). QuadraNdOptions/QuadraNdResult
// keep their own distinct type names rather than merging into
// QuadraOptions/QuadraResult, since their fields genuinely differ (this
// overload has no adaptive-refinement loop, so it tracks no error_estimate/
// refinements/converged the way the 1-D overload does).
//
// This inherits the curse of dimensionality any tensor-product rule has:
// total node count is the PRODUCT of every axis's own node count, so it
// grows exponentially with dimension -- fine for the low/moderate-D case
// (D <= ~5-6 with a modest per-axis resolution) this is meant for;
// `QuadraNdOptions::max_total_nodes` throws a clear error rather than
// silently trying to allocate an enormous grid if a caller asks for more
// than that. For genuinely high-dimensional integrals, use aleatoria()'s
// N-D Monte Carlo overload instead -- its accuracy is dimension-
// independent by construction, which a tensor-product rule's isn't.
//
// Deliberately finite-domain only for this pass: per-axis semi-infinite/
// infinite bounds (mirroring the 1-D overload's own substitution support)
// would need `fn` rewritten one axis at a time while leaving the others
// alone, which is a real, separate piece of work not attempted here.
struct QuadraNdOptions {
  QuadraRule rule{QuadraRule::GaussLegendre};
  std::int64_t resolution{5};    // nodes-per-axis (Trapezoidal/Simpson) or panels-per-axis (GaussLegendre)
  std::int64_t gauss_points{5};  // GaussLegendre: 5 or 10
  ScalarType dtype{ScalarType::Float64};
  pluss::codem::Platform platform{pluss::codem::Platform::CPU};
  std::int64_t max_total_nodes{2'000'000};
};

struct QuadraNdResult {
  Series value = pluss::series::undefined();
  std::int64_t dimensions{0};
  std::int64_t evaluations{0};
};

namespace detail {

// Per-axis nodes+weights for the ND tensor-product rule -- mirrors the
// SAME formulas as quadra_trapezoidal_/quadra_simpson_/
// quadra_gauss_legendre_ above, kept as a separate, self-contained
// generator (rather than refactoring those already-extensively-tested 1-D
// functions to share it) to avoid any risk of regressing them for a
// capability they don't need.
inline void quadra_1d_nodes_weights_(QuadraRule rule, double a, double b, std::int64_t resolution,
                                     std::int64_t gauss_points, std::vector<double>* nodes,
                                     std::vector<double>* weights) {
  switch (rule) {
    case QuadraRule::Trapezoidal: {
      std::int64_t n = resolution;
      if (n < 1) n = 1;
      const double h = (b - a) / static_cast<double>(n);
      nodes->resize(static_cast<std::size_t>(n + 1));
      weights->resize(static_cast<std::size_t>(n + 1));
      for (std::int64_t i = 0; i <= n; ++i) {
        (*nodes)[static_cast<std::size_t>(i)] = a + h * static_cast<double>(i);
        (*weights)[static_cast<std::size_t>(i)] = (i == 0 || i == n) ? 0.5 * h : h;
      }
      break;
    }
    case QuadraRule::Simpson: {
      std::int64_t n = resolution;
      if (n < 2) n = 2;
      if (n % 2 != 0) ++n;
      const double h = (b - a) / static_cast<double>(n);
      nodes->resize(static_cast<std::size_t>(n + 1));
      weights->resize(static_cast<std::size_t>(n + 1));
      for (std::int64_t i = 0; i <= n; ++i) {
        (*nodes)[static_cast<std::size_t>(i)] = a + h * static_cast<double>(i);
        double w = (i % 2 == 0) ? 2.0 : 4.0;
        if (i == 0 || i == n) w = 1.0;
        (*weights)[static_cast<std::size_t>(i)] = w * h / 3.0;
      }
      break;
    }
    case QuadraRule::GaussLegendre:
    default: {
      std::int64_t panels = resolution;
      if (panels < 1) panels = 1;
      const GaussLegendreTable& table = gauss_legendre_table_(gauss_points);
      const std::size_t per_panel = table.nodes.size();
      const double panel_width = (b - a) / static_cast<double>(panels);
      nodes->resize(static_cast<std::size_t>(panels) * per_panel);
      weights->resize(static_cast<std::size_t>(panels) * per_panel);
      for (std::int64_t p = 0; p < panels; ++p) {
        const double lo = a + static_cast<double>(p) * panel_width;
        const double mid = lo + panel_width * 0.5;
        const double half = panel_width * 0.5;
        for (std::size_t k = 0; k < per_panel; ++k) {
          const std::size_t idx = static_cast<std::size_t>(p) * per_panel + k;
          (*nodes)[idx] = mid + half * table.nodes[k];
          (*weights)[idx] = half * table.weights[k];
        }
      }
      break;
    }
  }
}

}  // namespace detail

inline QuadraNdResult quadra(const IntegrandNdFn& fn, const std::vector<std::pair<double, double>>& bounds,
                             const QuadraNdOptions& options = {}) {
  if (!fn) {
    ::pluss::throw_error("subsp::math::integ::quadra: integrand callback is required");
  }
  if (bounds.empty()) {
    ::pluss::throw_error("subsp::math::integ::quadra: expected at least one bound pair");
  }
  const auto D = static_cast<std::int64_t>(bounds.size());

  std::vector<std::vector<double>> per_axis_nodes(static_cast<std::size_t>(D));
  std::vector<std::vector<double>> per_axis_weights(static_cast<std::size_t>(D));
  std::int64_t total = 1;
  for (std::int64_t d = 0; d < D; ++d) {
    const auto& [lo, hi] = bounds[static_cast<std::size_t>(d)];
    if (!(hi > lo)) {
      ::pluss::throw_error("subsp::math::integ::quadra: expected hi > lo for every bound pair");
    }
    if (std::isinf(lo) || std::isinf(hi)) {
      ::pluss::throw_error(
          "subsp::math::integ::quadra: infinite bounds are not supported per-axis for the N-D overload (unlike "
          "the 1-D overload's case) -- use aleatoria()'s N-D overload for an infinite-domain multi-dimensional "
          "integral instead");
    }
    detail::quadra_1d_nodes_weights_(options.rule, lo, hi, options.resolution, options.gauss_points,
                                     &per_axis_nodes[static_cast<std::size_t>(d)],
                                     &per_axis_weights[static_cast<std::size_t>(d)]);
    total *= static_cast<std::int64_t>(per_axis_nodes[static_cast<std::size_t>(d)].size());
    if (total > options.max_total_nodes) {
      ::pluss::throw_error(
          "subsp::math::integ::quadra: tensor-product node count exceeds max_total_nodes -- reduce "
          "resolution/dimensions (deterministic ND quadrature scales exponentially with dimension), or use "
          "aleatoria()'s N-D Monte Carlo overload instead for high-dimensional integrals");
    }
  }

  // Cartesian product ("meshgrid, then flatten"): one flat coordinate
  // array per axis (length `total`) plus one flat combined-weight array,
  // via mixed-radix counting over the per-axis node counts.
  std::vector<std::vector<double>> coords(static_cast<std::size_t>(D), std::vector<double>(static_cast<std::size_t>(total)));
  std::vector<double> combined_weight(static_cast<std::size_t>(total), 1.0);
  for (std::int64_t idx = 0; idx < total; ++idx) {
    std::int64_t rem = idx;
    for (std::int64_t d = D - 1; d >= 0; --d) {
      const auto nd = static_cast<std::int64_t>(per_axis_nodes[static_cast<std::size_t>(d)].size());
      const std::int64_t k = rem % nd;
      rem /= nd;
      coords[static_cast<std::size_t>(d)][static_cast<std::size_t>(idx)] =
          per_axis_nodes[static_cast<std::size_t>(d)][static_cast<std::size_t>(k)];
      combined_weight[static_cast<std::size_t>(idx)] *= per_axis_weights[static_cast<std::size_t>(d)][static_cast<std::size_t>(k)];
    }
  }

  std::vector<Series> x;
  x.reserve(static_cast<std::size_t>(D));
  for (std::int64_t d = 0; d < D; ++d) {
    x.push_back(pluss::make_series(coords[static_cast<std::size_t>(d)], {total}).to(options.dtype).to(options.platform));
  }
  const Series w = pluss::make_series(combined_weight, {total}).to(options.dtype).to(options.platform);

  const Series y = fn(x);
  detail::require_defined_(y, "quadra");

  QuadraNdResult result;
  result.dimensions = D;
  result.evaluations = total;
  result.value = y.mul(w).sum(y.dim() - 1);
  return result;
}

// ---------------------------------------------------------------------
// quadra_scalar: the scalar (plain double-in/double-out) counterpart of
// quadra, for integrands genuinely outside the Series/autodiff machinery
// (e.g. PlussMa/Stats's own CDF-from-PDF helpers, which evaluate a mixture
// PDF at one scalar point via ordinary double arithmetic, not Series ops
// -- forcing those through the batched Series engine above would gain
// nothing and cost a Series round-trip per call). Fixed-node by default
// (matching the two duplicated helpers this replaces bit-for-bit at their
// existing call sites), with the same adaptive step-doubling option quadra
// has available if a caller wants it.
// ---------------------------------------------------------------------

struct QuadraScalarOptions {
  std::int64_t nodes{256};
  bool adaptive{false};
  double tolerance{1e-10};
  std::int64_t max_refinements{8};
};

struct QuadraScalarResult {
  double value{0.0};
  double error_estimate{0.0};
  std::int64_t evaluations{0};
  bool converged{false};
};

namespace detail {

inline double quadra_composite_simpson_scalar_(const ScalarIntegrandFn& fn, double a, double b, std::int64_t n) {
  if (b <= a) return 0.0;
  if (n < 2) n = 2;
  if (n % 2 != 0) ++n;
  const double h = (b - a) / static_cast<double>(n);
  double acc = fn(a) + fn(b);
  for (std::int64_t i = 1; i < n; ++i) {
    const double x = a + h * static_cast<double>(i);
    acc += fn(x) * ((i % 2 == 0) ? 2.0 : 4.0);
  }
  return acc * h / 3.0;
}

}  // namespace detail

inline QuadraScalarResult quadra_scalar(const ScalarIntegrandFn& fn, double a, double b,
                                        const QuadraScalarOptions& options = {}) {
  if (!fn) {
    ::pluss::throw_error("subsp::math::integ::quadra_scalar: integrand callback is required");
  }
  if (options.nodes < 2) {
    ::pluss::throw_error("subsp::math::integ::quadra_scalar: nodes must be >= 2");
  }
  QuadraScalarResult result;
  if (b <= a) {
    result.converged = true;
    return result;
  }

  std::int64_t n = options.nodes;
  double value = detail::quadra_composite_simpson_scalar_(fn, a, b, n);
  result.evaluations += n + 1;

  if (!options.adaptive) {
    result.value = value;
    result.converged = true;
    return result;
  }

  for (std::int64_t r = 0; r < options.max_refinements; ++r) {
    n *= 2;
    const double refined = detail::quadra_composite_simpson_scalar_(fn, a, b, n);
    result.evaluations += n + 1;
    const double err = std::abs(refined - value);
    result.error_estimate = err;
    value = refined;
    if (err <= options.tolerance * std::max(1.0, std::abs(refined))) {
      result.converged = true;
      break;
    }
  }
  result.value = value;
  return result;
}

// ---------------------------------------------------------------------
// Aleatoria: Monte Carlo integration. Vectorized: every sample is drawn
// and evaluated in one batched call (via Series::rand's own native RNG
// kernel), never a per-sample loop -- the only way Monte Carlo's typical
// sample counts (10^5-10^7+) stay fast.
// ---------------------------------------------------------------------

struct AleatoriaOptions {
  std::int64_t samples{1'000'000};
  std::uint64_t seed{0};
  ScalarType dtype{ScalarType::Float64};
  pluss::codem::Platform platform{pluss::codem::Platform::CPU};
  // Simple stratified-jitter sampling (u_i = (i + U(0,1))/N instead of
  // plain iid U(0,1)) -- a cheap, well-known variance-reduction technique:
  // guarantees one sample per equal-width stratum instead of leaving
  // clumping/gaps to chance, at zero extra evaluations.
  bool stratified{false};
  // Only used when `a`/`b` passed to the 1-D aleatoria() overload is
  // +-infinity -- see QuadraOptions::infinite_domain_epsilon (identical
  // meaning, same change-of-variables substitutions).
  double infinite_domain_epsilon{1e-10};
};

struct AleatoriaResult {
  Series value = pluss::series::undefined();
  double standard_error{0.0};
  std::int64_t samples{0};
};

namespace detail {

// Shared mean/standard-error reduction for both the 1-D and N-D
// aleatoria() overloads. `scaled` is the per-sample Monte Carlo estimator
// contribution (integrand value times the domain volume); dtype-agnostic
// via the abs2()-based variance formula for a possibly-complex integrand
// -- Var(X) = E[|X|^2] - |E[X]|^2, the correct complex variance (matching
// this codebase's established |z|^2-not-z^2 discipline for complex second
// moments, e.g. the optimizer-layer view_as_real_ work), rather than the
// real-only Series::std()/var() (which this deliberately avoids calling on
// a complex Series at all, sidestepping the question of whether it even
// has defined complex-dtype behavior).
inline AleatoriaResult aleatoria_reduce_(const Series& scaled, std::int64_t samples) {
  AleatoriaResult result;
  result.samples = samples;
  result.value = scaled.mean();

  double variance;
  if (is_complex_(scaled)) {
    const double mean_abs2 = scalar_magnitude_(result.value.abs2());
    const double mean_of_abs2 = scaled.abs2().mean().to(pluss::pCPU, ScalarType::Float64).item<double>();
    variance = std::max(0.0, mean_of_abs2 - mean_abs2);
  } else {
    const Series sample_std = scaled.std(true);
    const double sd = sample_std.to(pluss::pCPU, ScalarType::Float64).item<double>();
    variance = sd * sd;
  }
  result.standard_error = std::sqrt(variance / static_cast<double>(samples));
  return result;
}

}  // namespace detail

inline AleatoriaResult aleatoria(const IntegrandFn& fn, double a, double b, const AleatoriaOptions& options = {}) {
  if (!fn) {
    ::pluss::throw_error("subsp::math::integ::aleatoria: integrand callback is required");
  }
  if (options.samples <= 0) {
    ::pluss::throw_error("subsp::math::integ::aleatoria: samples must be > 0");
  }

  // Same substitution-and-recurse treatment as quadra() -- see its own
  // comment above the substitution helpers for the full rationale. The
  // recursive call always has finite bounds, so this terminates in
  // exactly one extra call.
  const bool a_inf = std::isinf(a);
  const bool b_inf = std::isinf(b);
  if (a_inf || b_inf) {
    const double eps = options.infinite_domain_epsilon;
    if (a_inf && b_inf) {
      if (!(a < 0.0) || !(b > 0.0)) {
        ::pluss::throw_error("subsp::math::integ::aleatoria: expected a = -infinity, b = +infinity for a fully "
                             "infinite domain");
      }
      return aleatoria(detail::substitute_both_infinite_(fn), -1.0 + eps, 1.0 - eps, options);
    }
    if (b_inf) {
      if (!(b > 0.0)) {
        ::pluss::throw_error(
            "subsp::math::integ::aleatoria: expected b = +infinity for a semi-infinite upper bound");
      }
      return aleatoria(detail::substitute_upper_infinite_(fn, a), 0.0, 1.0 - eps, options);
    }
    if (!(a < 0.0)) {
      ::pluss::throw_error("subsp::math::integ::aleatoria: expected a = -infinity for a semi-infinite lower bound");
    }
    return aleatoria(detail::substitute_lower_infinite_(fn, b), 0.0, 1.0 - eps, options);
  }

  if (!(b > a)) {
    ::pluss::throw_error("subsp::math::integ::aleatoria: expected b > a");
  }

  const Config cfg = Config{}.with_dtype(options.dtype).with_platform(options.platform);
  Series u = pluss::series::rand({options.samples}, cfg, options.seed);
  if (options.stratified) {
    const Series idx = Series::linspace(0.0, static_cast<double>(options.samples - 1), options.samples,
                                        options.dtype, options.platform);
    u = idx.add(u).div(static_cast<double>(options.samples));
  }
  const Series x = u.mul(b - a).add(a);
  const Series y = fn(x);
  detail::require_defined_(y, "aleatoria");
  const Series scaled = y.mul(b - a);
  return detail::aleatoria_reduce_(scaled, options.samples);
}

inline AleatoriaResult aleatoria(const IntegrandNdFn& fn, const std::vector<std::pair<double, double>>& bounds,
                                 const AleatoriaOptions& options = {}) {
  if (!fn) {
    ::pluss::throw_error("subsp::math::integ::aleatoria: integrand callback is required");
  }
  if (bounds.empty()) {
    ::pluss::throw_error("subsp::math::integ::aleatoria: expected at least one bound pair");
  }
  if (options.samples <= 0) {
    ::pluss::throw_error("subsp::math::integ::aleatoria: samples must be > 0");
  }

  const Config cfg = Config{}.with_dtype(options.dtype).with_platform(options.platform);
  std::vector<Series> coords;
  coords.reserve(bounds.size());
  double volume = 1.0;
  // A distinct RNG stream per axis (large odd offset, Weyl-sequence-style
  // decorrelation) -- reusing the same seed per dimension would correlate
  // every axis's samples with each other, silently biasing the estimate
  // toward the diagonal instead of covering the domain uniformly.
  for (std::size_t d = 0; d < bounds.size(); ++d) {
    const auto& [lo, hi] = bounds[d];
    if (!(hi > lo)) {
      ::pluss::throw_error("subsp::math::integ::aleatoria: expected hi > lo for every bound pair");
    }
    volume *= (hi - lo);
    const std::uint64_t axis_seed = options.seed + static_cast<std::uint64_t>(d) * 0x9E3779B97F4A7C15ULL;
    const Series u = pluss::series::rand({options.samples}, cfg, axis_seed);
    coords.push_back(u.mul(hi - lo).add(lo));
  }
  const Series y = fn(coords);
  detail::require_defined_(y, "aleatoria");
  const Series scaled = y.mul(volume);
  return detail::aleatoria_reduce_(scaled, options.samples);
}

// ---------------------------------------------------------------------
// Analytica: closed-form solutions for named integrand families. Plain
// double in, double out -- these are exact formulas over scalar bounds/
// parameters, with no Series or autodiff involved (there is nothing to
// batch or differentiate: the whole point is that the answer is already
// known in closed form). See the module-level comment above for why this
// is a directly-invoked registry rather than something Integra's strategy
// dispatch ever tries automatically.
// ---------------------------------------------------------------------

namespace analytica {

// Integral(a,b) exp(k*x) dx = (exp(k*b) - exp(k*a)) / k  (k != 0);
// degenerates to (b - a) at k == 0 (the constant-1 integrand).
inline double exponential(double k, double a, double b) {
  if (k == 0.0) return b - a;
  return (std::exp(k * b) - std::exp(k * a)) / k;
}

// Integral(a,b) x^n dx = (b^(n+1) - a^(n+1)) / (n+1)  (n != -1);
// the n == -1 case is the ordinary ln(x) antiderivative, ln|b| - ln|a|,
// which requires a and b nonzero and on the same side of zero (the
// integrand itself is undefined at x == 0, so any interval straddling it
// has no finite integral either way).
inline double power(double n, double a, double b) {
  if (std::abs(n + 1.0) < 1e-12) {
    if (a == 0.0 || b == 0.0 || (a < 0.0) != (b < 0.0)) {
      ::pluss::throw_error(
          "subsp::math::integ::analytica::power: n == -1 requires a and b nonzero and on the same side of 0");
    }
    return std::log(std::abs(b)) - std::log(std::abs(a));
  }
  return (std::pow(b, n + 1.0) - std::pow(a, n + 1.0)) / (n + 1.0);
}

// Integral(a,b) sum_i coeffs[i] * x^i dx, coeffs = [c0, c1, .., cn] -- term
// by term via power() above, exact for every term (never hits the n == -1
// branch since polynomial exponents are always >= 0).
inline double polynomial(const std::vector<double>& coeffs, double a, double b) {
  double total = 0.0;
  for (std::size_t i = 0; i < coeffs.size(); ++i) {
    if (coeffs[i] == 0.0) continue;
    total += coeffs[i] * power(static_cast<double>(i), a, b);
  }
  return total;
}

// Integral(a,b) exp(-((x-mean)^2) / (2*stddev^2)) dx -- the UNnormalized
// Gaussian kernel (no 1/(stddev*sqrt(2*pi)) prefactor), via the error
// function: substituting z = (x-mean)/(stddev*sqrt2) turns this into the
// standard erf antiderivative.
inline double gaussian(double a, double b, double mean = 0.0, double stddev = 1.0) {
  if (!(stddev > 0.0)) {
    ::pluss::throw_error("subsp::math::integ::analytica::gaussian: stddev must be positive");
  }
  constexpr double kInvSqrt2 = 0.70710678118654752440;
  constexpr double kSqrtTwoPi = 2.50662827463100050242;
  const double za = (a - mean) * kInvSqrt2 / stddev;
  const double zb = (b - mean) * kInvSqrt2 / stddev;
  return stddev * kSqrtTwoPi * 0.5 * (std::erf(zb) - std::erf(za));
}

// Integral(a,b) of the NORMALIZED Gaussian PDF -- a genuine probability
// mass in [0, 1] (the CDF difference Phi(b) - Phi(a)) -- exactly
// gaussian(...) above divided by the normal PDF's own normalizing
// constant.
inline double gaussian_probability(double a, double b, double mean = 0.0, double stddev = 1.0) {
  if (!(stddev > 0.0)) {
    ::pluss::throw_error("subsp::math::integ::analytica::gaussian_probability: stddev must be positive");
  }
  constexpr double kSqrtTwoPi = 2.50662827463100050242;
  return gaussian(a, b, mean, stddev) / (stddev * kSqrtTwoPi);
}

// Integral(a,b) sin(k*x) dx = (cos(k*a) - cos(k*b)) / k  (k != 0);
// degenerates to 0 at k == 0 (sin(0*x) == 0 everywhere).
inline double sine(double k, double a, double b) {
  if (k == 0.0) return 0.0;
  return (std::cos(k * a) - std::cos(k * b)) / k;
}

// Integral(a,b) cos(k*x) dx = (sin(k*b) - sin(k*a)) / k  (k != 0);
// degenerates to (b - a) at k == 0 (cos(0*x) == 1 everywhere).
inline double cosine(double k, double a, double b) {
  if (k == 0.0) return b - a;
  return (std::sin(k * b) - std::sin(k * a)) / k;
}

// Integral(a,b) 1 / (x^2 + scale^2) dx = (1/scale) * (atan(b/scale) -
// atan(a/scale)) -- the (unnormalized) Cauchy/Lorentzian kernel, the same
// family of characteristic-function-adjacent integrand this module's
// semi-infinite/infinite substitutions target (e.g. the Cauchy
// distribution's own CDF, or a Lorentzian spectral-line-shape integral).
inline double lorentzian(double scale, double a, double b) {
  if (!(scale > 0.0)) {
    ::pluss::throw_error("subsp::math::integ::analytica::lorentzian: scale must be positive");
  }
  return (std::atan(b / scale) - std::atan(a / scale)) / scale;
}

// Integral(a,b) sinh(k*x) dx = (cosh(k*b) - cosh(k*a)) / k  (k != 0);
// degenerates to 0 at k == 0 (sinh(0*x) == 0 everywhere).
inline double hyperbolic_sine(double k, double a, double b) {
  if (k == 0.0) return 0.0;
  return (std::cosh(k * b) - std::cosh(k * a)) / k;
}

// Integral(a,b) cosh(k*x) dx = (sinh(k*b) - sinh(k*a)) / k  (k != 0);
// degenerates to (b - a) at k == 0 (cosh(0*x) == 1 everywhere).
inline double hyperbolic_cosine(double k, double a, double b) {
  if (k == 0.0) return b - a;
  return (std::sinh(k * b) - std::sinh(k * a)) / k;
}

// Integral(a,b) tanh(x) dx = ln(cosh(b)) - ln(cosh(a)).
inline double hyperbolic_tangent(double a, double b) { return std::log(std::cosh(b)) - std::log(std::cosh(a)); }

// Numerically stable softplus, ln(1 + e^x) without overflowing for large
// |x| -- shared by sigmoid() below (the antiderivative of the logistic
// sigmoid IS softplus, directly relevant to this codebase's own
// activation-function surface).
inline double softplus_stable_(double x) { return std::max(x, 0.0) + std::log1p(std::exp(-std::abs(x))); }

// Integral(a,b) sigmoid(x) dx = softplus(b) - softplus(a), where
// sigmoid(x) = 1/(1+exp(-x)) and softplus(x) = ln(1+exp(x)) is its exact
// antiderivative (d/dx softplus(x) = sigmoid(x)) -- the logistic-sigmoid
// counterpart to gaussian_probability() above.
inline double sigmoid(double a, double b) { return softplus_stable_(b) - softplus_stable_(a); }

// Integral(a,b) (x-mean)*exp(-(x-mean)^2/(2*stddev^2)) dx = stddev^2 *
// (exp(-(a-mean)^2/(2*stddev^2)) - exp(-(b-mean)^2/(2*stddev^2))) -- the
// first-moment companion to gaussian()/gaussian_probability() above (e.g.
// for computing E[X * 1{X in [a,b]}] of a truncated normal).
inline double gaussian_moment(double a, double b, double mean = 0.0, double stddev = 1.0) {
  if (!(stddev > 0.0)) {
    ::pluss::throw_error("subsp::math::integ::analytica::gaussian_moment: stddev must be positive");
  }
  const double var = stddev * stddev;
  const double za = (a - mean) * (a - mean) / (2.0 * var);
  const double zb = (b - mean) * (b - mean) / (2.0 * var);
  return var * (std::exp(-za) - std::exp(-zb));
}

// Integral(a,b) 1 / sqrt(1 - x^2) dx = asin(b) - asin(a)  (requires
// |a| <= 1, |b| <= 1 -- the integrand itself is undefined outside [-1,1]).
inline double arcsine_kernel(double a, double b) {
  if (std::abs(a) > 1.0 || std::abs(b) > 1.0) {
    ::pluss::throw_error("subsp::math::integ::analytica::arcsine_kernel: a and b must both lie in [-1, 1]");
  }
  return std::asin(b) - std::asin(a);
}

}  // namespace analytica

// ---------------------------------------------------------------------
// Integra: the top-level convenience dispatcher. Picks a strategy (or
// honors an explicit one) and returns a uniform IntegraResult regardless
// of which engine actually ran.
// ---------------------------------------------------------------------

enum class IntegraStrategy : std::uint8_t {
  Quadrature = 0,
  MonteCarlo = 1,
  Auto = 2,
};

struct IntegraOptions {
  IntegraStrategy strategy{IntegraStrategy::Auto};
  QuadraOptions quadrature{};
  AleatoriaOptions monte_carlo{};
};

struct IntegraResult {
  Series value = pluss::series::undefined();
  double error_estimate{0.0};
  std::int64_t evaluations{0};
  IntegraStrategy strategy_used{IntegraStrategy::Quadrature};
  bool converged{false};
};

inline IntegraResult integra(const IntegrandFn& fn, double a, double b, const IntegraOptions& options = {}) {
  IntegraResult result;

  if (options.strategy == IntegraStrategy::MonteCarlo) {
    const AleatoriaResult mc = aleatoria(fn, a, b, options.monte_carlo);
    result.value = mc.value;
    result.error_estimate = mc.standard_error;
    result.evaluations = mc.samples;
    result.strategy_used = IntegraStrategy::MonteCarlo;
    result.converged = true;
    return result;
  }

  if (options.strategy == IntegraStrategy::Quadrature) {
    const QuadraResult q = quadra(fn, a, b, options.quadrature);
    result.value = q.value;
    result.error_estimate = q.error_estimate;
    result.evaluations = q.evaluations;
    result.strategy_used = IntegraStrategy::Quadrature;
    result.converged = q.converged;
    return result;
  }

  // Auto: try Quadrature first -- composite quadrature converges faster
  // and to tighter accuracy than Monte Carlo for a smooth, low-dimensional
  // integrand, which is exactly what a single [a, b] interval is. If it
  // genuinely fails to converge within its own adaptive refinement budget
  // (options.quadrature.max_refinements at options.quadrature.tolerance),
  // that is a REAL, runtime-observed signal that the smoothness assumption
  // behind quadrature isn't holding for this particular fn -- at which
  // point escalate to Monte Carlo, whose convergence doesn't depend on
  // smoothness at all. This is dynamic in the only sense actually
  // available here (did the attempted method converge?), not because fn's
  // structure is inspected -- it's an opaque callable (see the
  // IntegrandFn comment above), so there is no way to decide "this needs
  // Monte Carlo" ahead of actually trying quadrature and watching it fail.
  // Monte Carlo's OTHER real advantage -- dimension-independent
  // convergence -- only shows up in genuinely high-dimensional integrals,
  // which is why the N-D aleatoria() overload exists as its own explicit
  // entry point rather than something this 1-D Auto would ever reach for.
  const QuadraResult q = quadra(fn, a, b, options.quadrature);
  if (q.converged) {
    result.value = q.value;
    result.error_estimate = q.error_estimate;
    result.evaluations = q.evaluations;
    result.strategy_used = IntegraStrategy::Quadrature;
    result.converged = true;
    return result;
  }

  const AleatoriaResult mc = aleatoria(fn, a, b, options.monte_carlo);
  result.value = mc.value;
  result.error_estimate = mc.standard_error;
  result.evaluations = q.evaluations + mc.samples;
  result.strategy_used = IntegraStrategy::MonteCarlo;
  result.converged = true;
  return result;
}

}  // namespace pluss::subsp::math::integ
