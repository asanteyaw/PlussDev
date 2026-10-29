#pragma once

// pluss::subsp::math::integ::lebesgue -- eager Lebesgue integration by range partition (the layer-cake
// decomposition), and measure_of for the measure of a super-level set.
//
// The Riemann/quadrature engines partition the DOMAIN. This one partitions the RANGE: with levels
// 0 = t_0 < t_1 < ... < t_K = max f, the simple functions
//     s_lo = sum_j t_j 1{t_j <= f < t_{j+1}}     and     s_hi = sum_j t_{j+1} 1{t_j < f <= t_{j+1}}
// bracket f, so  int s_lo dmu <= int f dmu <= int s_hi dmu, and the bracket width is (max f / K) mu(A).
// The number of levels is chosen so that the bracket is narrower than the tolerance; the bracket is
// reported, so a result is never returned as exact without the evidence for it.
//
// Scope, stated plainly. The measure here is Lebesgue measure on [a, b], discretized into graded cells
// (the cell count doubles until the value stops changing), so the bracket certifies the RANGE partition
// error only; the domain discretization error is the reported refinement difference. This is an eager
// numerical evaluator and is not an operator: operators in Math/Calculus return analytic Functions.
// Measures other than Lebesgue are the operators in operators::measure.

#include "pluss/subsp/math/integ.h"

namespace pluss::subsp::math::integ {

struct LebesgueOptions {
  std::int64_t cells{1024};            // initial number of cells; doubled on each refinement
  std::int64_t max_refinements{8};
  // Cells are x = a + (b - a) u^grading on a uniform u: grading > 1 concentrates cells at `a`
  // (endpoint singularities such as 1 / sqrt(x) on (0, 1]).
  double grading{1.0};
  double tolerance{1e-8};
  ScalarType dtype{ScalarType::Float64};
  pluss::codem::Platform platform{pluss::codem::Platform::CPU};
};

struct LebesgueResult {
  Series value = pluss::series::undefined();
  Series lower = pluss::series::undefined();   // integral of the lower simple function (last refinement)
  Series upper = pluss::series::undefined();   // integral of the upper simple function
  double error_estimate{std::numeric_limits<double>::infinity()};  // half bracket + refinement change
  std::int64_t evaluations{0};
  std::int64_t refinements{0};
  bool converged{false};
  // Value after each refinement for the first batch element, and whether that sequence was
  // non-decreasing (the monotone-convergence signature for an increasing sequence of simple
  // approximations). A diagnostic about the numerical sequence, not a proof.
  std::vector<double> history;
  bool monotone{true};
};

struct MeasureResult {
  Series value = pluss::series::undefined();
  double error_estimate{std::numeric_limits<double>::infinity()};
  std::int64_t evaluations{0};
  std::int64_t refinements{0};
  bool converged{false};
};

namespace detail {

struct LebesgueGrid_ {
  Series x = pluss::series::undefined();       // cell midpoints [n]
  Series width = pluss::series::undefined();   // cell widths [n]
};

inline LebesgueGrid_ lebesgue_grid_(double a, double b, std::int64_t n, double grading, ScalarType dtype,
                                    pluss::codem::Platform platform) {
  const Series edges = Series::linspace(0.0, 1.0, n + 1, dtype, platform);
  const Series mids = Series::linspace(0.0, 1.0, n + 1, dtype, platform).slice(0, 0, n)
                          .add(0.5 / static_cast<double>(n));
  const auto map = [&](const Series& u) { return grading == 1.0 ? u.mul(b - a).add(a) : u.pow(grading).mul(b - a).add(a); };
  const Series e = map(edges);
  return {map(mids), e.slice(0, 1, n + 1).sub(e.slice(0, 0, n))};
}

// Reduces the last axis of a [..., n] integrand to [batch], with the range partition of its positive and
// negative parts; returns {value, lower, upper} as [batch] Series.
//
// The partition of the range of a non-negative part p is 0, then the geometric levels
// t_k = y_min (1 + eps)^k up to max p. A point in [t_k, t_{k+1}) is bracketed by (t_k, t_{k+1}), so the
// bracket is within a factor (1 + eps) of the integral, which makes its width relative to the integral:
// it does not blow up for a tall, thin integrand such as 1 / sqrt(x) near 0, where a uniform partition of
// [0, max f] would need astronomically many levels. Values below y_min = tol / (4 mu(A)) are bracketed by
// (0, y_min), costing at most tol / 4 in total; eps is chosen so the relative part costs at most tol / 4.
inline void lebesgue_reduce_(const Series& y2, const Series& w, double tolerance, Series* value, Series* lower,
                             Series* upper) {
  const double measure = w.sum().item<double>();
  const double y_min = tolerance * 0.25 / measure;
  const auto part = [&](const Series& p, Series* lo, Series* hi) {
    const Series estimate = p.mul(w).sum(1, true);                                    // [B, 1], midpoint sum
    const Series eps = estimate.clamp_min(tolerance).reciprocal().mul(tolerance * 0.25).clamp_max(0.5);
    const Series log_step = eps.log1p();                                              // [B, 1]
    const Series above = p.gt(y_min).to(p.scalar_type());                             // 1 where p > y_min
    const Series q = p.clamp_min(y_min).div(y_min).log().div(log_step);
    const Series low_level = q.floor().mul(log_step).exp().mul(y_min);
    const Series high_level = q.ceil().mul(log_step).exp().mul(y_min);
    const Series positive = p.gt(0.0).to(p.scalar_type());
    *lo = low_level.mul(above).mul(w).sum(1, false);
    *hi = high_level.mul(above).add(above.mul(-1.0).add(1.0).mul(positive).mul(y_min)).mul(w).sum(1, false);
  };
  Series plo = pluss::series::undefined(), phi = pluss::series::undefined();
  Series nlo = pluss::series::undefined(), nhi = pluss::series::undefined();
  part(y2.clamp_min(0.0), &plo, &phi);
  part(y2.mul(-1.0).clamp_min(0.0), &nlo, &nhi);
  *lower = plo.sub(nhi);
  *upper = phi.sub(nlo);
  *value = (*lower).add(*upper).mul(0.5);
}

}  // namespace detail

inline LebesgueResult lebesgue(const IntegrandFn& fn, double a, double b, const LebesgueOptions& options = {}) {
  if (!fn) ::pluss::throw_error("subsp::math::integ::lebesgue: integrand callback is required");
  if (!(std::isfinite(a) && std::isfinite(b) && a < b))
    ::pluss::throw_error("subsp::math::integ::lebesgue: needs finite a < b");
  if (!(options.tolerance > 0.0) || options.cells < 2 || options.grading <= 0.0)
    ::pluss::throw_error("subsp::math::integ::lebesgue: tolerance > 0, cells >= 2 and grading > 0 are required");

  LebesgueResult result;
  Series previous = pluss::series::undefined();
  std::vector<std::int64_t> lead;
  std::int64_t n = options.cells;
  for (std::int64_t level = 0; level <= options.max_refinements; ++level, n *= 2) {
    const detail::LebesgueGrid_ grid = detail::lebesgue_grid_(a, b, n, options.grading, options.dtype, options.platform);
    const Series y = fn(grid.x);
    detail::require_defined_(y, "lebesgue");
    if (y.is_complex())
      ::pluss::throw_error("subsp::math::integ::lebesgue: integrate the real and imaginary parts separately");
    result.evaluations += n;
    const auto sizes = y.sizes();
    lead.assign(sizes.begin(), sizes.end() - 1);
    const std::int64_t batch = y.numel() / n;
    const Series y2 = y.reshape({batch, n});
    Series value = pluss::series::undefined();
    detail::lebesgue_reduce_(y2, grid.width, options.tolerance, &value, &result.lower, &result.upper);
    result.value = value;
    result.refinements = level;
    const double first = value.select(0, 0).item<double>();
    if (!result.history.empty() && first < result.history.back() - options.tolerance) result.monotone = false;
    result.history.push_back(first);
    const double half = result.upper.sub(result.lower).amax(0, false).item<double>() * 0.5;
    double change = std::numeric_limits<double>::infinity();
    if (previous.defined()) change = value.sub(previous).abs().amax(0, false).item<double>();
    result.error_estimate = half + (std::isinf(change) ? 0.0 : change);
    const double scale = std::max(1.0, value.abs().amax(0, false).item<double>());
    previous = value;
    if (!std::isinf(change) && result.error_estimate <= options.tolerance * scale) {
      result.converged = true;
      break;
    }
  }
  const auto restore = [&](const Series& s) { return s.reshape(lead); };
  result.value = restore(result.value);
  result.lower = restore(result.lower);
  result.upper = restore(result.upper);
  return result;
}

// mu({x in [a, b] : f(x) > level}) for Lebesgue mu, interpolating linearly between cell midpoints so the
// boundary of the level set is located inside a cell (second-order in the cell width).
inline MeasureResult measure_of(const IntegrandFn& fn, double a, double b, double level,
                                const LebesgueOptions& options = {}) {
  if (!fn) ::pluss::throw_error("subsp::math::integ::measure_of: integrand callback is required");
  if (!(std::isfinite(a) && std::isfinite(b) && a < b && std::isfinite(level)))
    ::pluss::throw_error("subsp::math::integ::measure_of: needs finite a < b and a finite level");
  MeasureResult result;
  Series previous = pluss::series::undefined();
  std::vector<std::int64_t> lead;
  std::int64_t n = options.cells;
  for (std::int64_t step = 0; step <= options.max_refinements; ++step, n *= 2) {
    const detail::LebesgueGrid_ grid = detail::lebesgue_grid_(a, b, n, options.grading, options.dtype, options.platform);
    const Series y = fn(grid.x);
    detail::require_defined_(y, "measure_of");
    result.evaluations += n;
    const auto sizes = y.sizes();
    lead.assign(sizes.begin(), sizes.end() - 1);
    const Series y2 = y.reshape({y.numel() / n, n});
    const Series left = y2.slice(1, 0, n - 1), right = y2.slice(1, 1, n);
    const Series hi = left.maximum(right), lo = left.minimum(right);
    const Series length = grid.x.slice(0, 1, n).sub(grid.x.slice(0, 0, n - 1));
    const Series fraction = hi.sub(level).div(hi.sub(lo).add(1e-300)).clamp_min(0.0).minimum(
        pluss::series::full({1}, 1.0, Config{}.with_dtype(options.dtype).with_platform(options.platform)));
    Series total = fraction.mul(length).sum(1, false);
    const Series first = y2.slice(1, 0, 1).gt(level).to(options.dtype).squeeze(1).mul(grid.x.select(0, 0).item<double>() - a);
    const Series last = y2.slice(1, n - 1, n).gt(level).to(options.dtype).squeeze(1).mul(b - grid.x.select(0, n - 1).item<double>());
    total = total.add(first).add(last);
    result.value = total;
    result.refinements = step;
    double change = std::numeric_limits<double>::infinity();
    if (previous.defined()) change = total.sub(previous).abs().amax(0, false).item<double>();
    result.error_estimate = change;
    previous = total;
    if (!std::isinf(change) && change <= options.tolerance * (b - a)) {
      result.converged = true;
      break;
    }
  }
  result.value = result.value.reshape(lead);
  return result;
}

}  // namespace pluss::subsp::math::integ
