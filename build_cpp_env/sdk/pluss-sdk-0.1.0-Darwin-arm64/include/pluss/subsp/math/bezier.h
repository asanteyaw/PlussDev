#pragma once

// Piecewise quintic Bezier functions y(x): C^2-smooth curves defined by chained quintic Bezier segments whose control x are non-decreasing, evaluated
// together with their first and second derivatives for whole Series of abscissae at once. This is the representation of the OpenSim / Millard muscle curves
// (OpenSim/Common SegmentedQuinticBezierToolkit and SmoothSegmentedFunction, read), kept here because it is a general numerical primitive (shape-preserving smooth
// curves with prescribed end slopes), not biology.
//
//   x(u) = sum_i B_i^5(u) X_i,  y(u) = sum_i B_i^5(u) Y_i,   u in [0, 1];   y(x) via u(x), dy/dx = y_u / x_u,   d2y/dx2 = (y_uu x_u - y_u x_uu) / x_u^3
//   outside [x_first, x_last] the function continues linearly with the prescribed end slopes (value and slope continuous, second derivative zero).
//   u(x) is found by safeguarded Newton (Newton step inside the current bracket, bisection otherwise): x(u) is monotone, so the iteration is globally convergent
//   and runs on all lanes at once.
//   `corner` builds the control points of a smooth "C-shaped" transition between two points with given slopes (curviness in [0, 1] places the middle
//   control points between the end points and the intersection of the two tangent lines).

#include "pluss/codem/codem.h"
#include "pluss/codem/series_factory.h"

#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace pluss::subsp::math::bezier {

using Control = std::array<double, 6>;

struct Segment final {
  Control x{}, y{};
};

// control points of a corner between (x0, y0, slope0) and (x1, y1, slope1)
[[nodiscard]] inline Segment corner(double x0, double y0, double dydx0, double x1, double y1, double dydx1, double curviness) {
  if (!(curviness >= 0.0 && curviness <= 1.0))
    throw std::invalid_argument("bezier::corner: curviness must lie in [0, 1]");
  double xc = 0.5 * (x0 + x1);
  if (std::abs(dydx0 - dydx1) > std::sqrt(2.220446049250313e-16))
    xc = (y1 - y0 - x1 * dydx1 + x0 * dydx0) / (dydx0 - dydx1);
  const double yc = (xc - x1) * dydx1 + y1;
  const double a = (xc - x0) * (xc - x0) + (yc - y0) * (yc - y0), b = (xc - x1) * (xc - x1) + (yc - y1) * (yc - y1), c = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
  if (!(c > a && c > b))
    throw std::invalid_argument("bezier::corner: the tangent lines do not form a C-shaped corner");
  const double x0m = x0 + curviness * (xc - x0), y0m = y0 + curviness * (yc - y0), x1m = x1 + curviness * (xc - x1), y1m = y1 + curviness * (yc - y1);
  return {{x0, x0m, x0m, x1m, x1m, x1}, {y0, y0m, y0m, y1m, y1m, y1}};
}

struct Curve final {
  std::vector<Segment> segments;
  double slope_below{0.0}; // dy/dx for x below the first control x
  double slope_above{0.0}; // dy/dx for x above the last control x
  [[nodiscard]] double x_first() const { return segments.front().x.front(); }
  [[nodiscard]] double x_last() const { return segments.back().x.back(); }
  [[nodiscard]] double y_first() const { return segments.front().y.front(); }
  [[nodiscard]] double y_last() const { return segments.back().y.back(); }
};

namespace detail {
// Bernstein -> power basis (a_k = sum_{i<=k} (-1)^{k-i} C(n,k) C(k,i) c_i), evaluated by Horner: a degree-5 polynomial costs 5 multiply-adds on the Series backend
template <std::size_t N>
[[nodiscard]] constexpr std::array<double, N> power_basis(const std::array<double, N> &c) {
  constexpr std::size_t n = N - 1;
  const auto binom = [](std::size_t nn, std::size_t kk) { double r = 1.0; for (std::size_t i = 1; i <= kk; ++i) r = r * static_cast<double>(nn - kk + i) / static_cast<double>(i); return r; };
  std::array<double, N> a{};
  for (std::size_t k = 0; k < N; ++k)
    for (std::size_t i = 0; i <= k; ++i)
      a[k] += (((k - i) % 2 == 0) ? 1.0 : -1.0) * binom(n, k) * binom(k, i) * c[i];
  return a;
}
template <std::size_t N>
[[nodiscard]] constexpr std::array<double, N - 1> derivative(const std::array<double, N> &a) {
  std::array<double, N - 1> d{};
  for (std::size_t k = 1; k < N; ++k) d[k - 1] = static_cast<double>(k) * a[k];
  return d;
}
template <std::size_t N>
[[nodiscard]] inline Series horner(const std::array<double, N> &a, const Series &u) {
  Series acc = u.mul(0.0).add(a[N - 1]);
  for (std::size_t k = N - 1; k-- > 0;) acc = acc.mul(u).add(a[k]);
  return acc;
}
struct Evaluation {
  Series value, d1, d2;
};
// Root u in [0, 1] of a monotone polynomial q(u) = target. A 32-interval table of q (host doubles, once per call) brackets the root to 1/32 for every lane (one
// broadcast comparison, no gather), then safeguarded Newton inside the bracket converges quadratically; the table is what makes control polygons with repeated points
// (dq/du ~ 0 mid-segment, as in the Millard curves) as cheap as smooth ones.
template <std::size_t N>
[[nodiscard]] inline Series solve_u(const std::array<double, N> &q, const Series &target, int iterations) {
  constexpr int k = 32;
  const auto dq = derivative(q);
  std::vector<double> table(k + 1);
  for (int i = 0; i <= k; ++i) {
    double v = q[N - 1];
    const double u = static_cast<double>(i) / k;
    for (std::size_t j = N - 1; j-- > 0;) v = v * u + q[j];
    table[static_cast<std::size_t>(i)] = v;
  }
  const bool increasing = table.back() >= table.front();
  const Series zero = target.mul(0.0);
  Series count = zero;                                       // number of table values passed by the target, i.e. the bracket index
  for (int i = 1; i < k; ++i) {
    const Series passed = increasing ? target.ge(table[static_cast<std::size_t>(i)]) : target.le(table[static_cast<std::size_t>(i)]);
    count = count.add(passed.to(target.scalar_type()));
  }
  Series lo = count.mul(1.0 / k), hi = count.add(1.0).mul(1.0 / k), u = lo.add(hi).mul(0.5);
  for (int it = 0; it < iterations; ++it) {
    const Series f = horner(q, u).sub(target);
    if (it % 4 == 3 && f.abs().max().item<double>() < 4e-16 * (1.0 + target.abs().max().item<double>())) break; // every lane converged to rounding level
    const Series above = increasing ? f.gt(0.0) : f.lt(0.0);   // the root lies below u
    hi = above.where(u, hi);
    lo = above.where(lo, u);
    const Series slope = horner(dq, u);
    const Series newton = u.sub(f.div(increasing ? slope.clamp(1e-300, 1e300) : slope.clamp(-1e300, -1e-300)));
    const Series inside = newton.gt(lo).logical_and(newton.lt(hi));
    u = inside.where(newton, lo.add(hi).mul(0.5));
  }
  return u;
}
[[nodiscard]] inline Evaluation at_u(const Segment &s, const Series &u) {
  const auto px = power_basis(s.x), py = power_basis(s.y);
  const auto dpx = derivative(px), dpy = derivative(py);
  const auto ddpx = derivative(dpx), ddpy = derivative(dpy);
  const Series xu = horner(dpx, u), yu = horner(dpy, u), xuu = horner(ddpx, u), yuu = horner(ddpy, u);
  return {horner(py, u), yu.div(xu), yuu.mul(xu).sub(yu.mul(xuu)).div(xu.square().mul(xu))};
}
[[nodiscard]] inline Evaluation segment_eval(const Segment &s, const Series &x, int iterations) { return at_u(s, solve_u(power_basis(s.x), x, iterations)); }
} // namespace detail

// y(x), y'(x), y''(x) for x of any shape
struct Result final {
  Series value, slope, curvature;
};
[[nodiscard]] inline Result evaluate(const Curve &curve, const Series &x, int iterations = 80) {
  if (curve.segments.empty())
    throw std::invalid_argument("bezier::evaluate: empty curve");
  const Series zero = x.mul(0.0);
  const double x0 = curve.x_first(), x1 = curve.x_last();
  const Series below = x.lt(x0), above = x.gt(x1);
  Series value = zero, slope = zero, curvature = zero;
  const Series inside_x = x.clamp(x0, x1);
  for (std::size_t k = 0; k < curve.segments.size(); ++k) {
    const Segment &seg = curve.segments[k];
    const bool last = k + 1 == curve.segments.size();
    const Series in_segment = last ? inside_x.ge(seg.x.front()) : inside_x.ge(seg.x.front()).logical_and(inside_x.lt(seg.x.back()));
    if (!in_segment.any().item<bool>()) continue;
    const detail::Evaluation e = detail::segment_eval(seg, inside_x.clamp(seg.x.front(), seg.x.back()), iterations);
    value = in_segment.where(e.value, value);
    slope = in_segment.where(e.d1, slope);
    curvature = in_segment.where(e.d2, curvature);
  }
  value = below.where(x.sub(x0).mul(curve.slope_below).add(curve.y_first()), above.where(x.sub(x1).mul(curve.slope_above).add(curve.y_last()), value));
  slope = below.where(zero.add(curve.slope_below), above.where(zero.add(curve.slope_above), slope));
  curvature = below.logical_or(above).where(zero, curvature);
  return {value, slope, curvature};
}
// y(x) only
[[nodiscard]] inline Series value(const Curve &curve, const Series &x) { return evaluate(curve, x).value; }

// inverse x(y) of a curve whose y is strictly increasing in x: the segment is chosen from the segment end values, u from the y polynomial, x from the x polynomial;
// y outside the range extrapolates linearly through the end slopes
[[nodiscard]] inline Series inverse(const Curve &curve, const Series &y, int iterations = 80) {
  const double y_first = curve.y_first(), y_last = curve.y_last();
  const Series yc = y.clamp(y_first, y_last);
  Series x = y.mul(0.0).add(curve.x_first());
  for (std::size_t k = 0; k < curve.segments.size(); ++k) {
    const Segment &seg = curve.segments[k];
    const bool last = k + 1 == curve.segments.size();
    const Series in_segment = last ? yc.ge(seg.y.front()) : yc.ge(seg.y.front()).logical_and(yc.lt(seg.y.back()));
    if (!in_segment.any().item<bool>()) continue;
    const Series u = detail::solve_u(detail::power_basis(seg.y), yc.clamp(seg.y.front(), seg.y.back()), iterations);
    x = in_segment.where(detail::horner(detail::power_basis(seg.x), u), x);
  }
  const Series left = y.sub(y_first).div(curve.slope_below).add(curve.x_first()), right = y.sub(y_last).div(curve.slope_above).add(curve.x_last());
  return y.lt(y_first).where(left, y.gt(y_last).where(right, x));
}

} // namespace pluss::subsp::math::bezier
