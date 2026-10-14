#pragma once

// Batched scalar root finding: every solver advances all lanes of a Series in
// the same Series ops (no per-lane host loops), so one call solves a whole
// parameter sweep. `f` maps a Series of any shape to a Series of that shape.
//
//   bisect    - bracketed; halves the interval of each lane (needs a sign change)
//   illinois  - bracketed regula falsi with the Illinois modification (superlinear,
//               never leaves the bracket); the bracketed workhorse
//   newton    - Newton-Raphson with an analytic derivative or a central difference
//   secant    - derivative-free
//
// Fixed iteration counts keep the loop free of host synchronisation except for the
// optional early exit (checked every `check_every` iterations). Results report the
// final residual so callers can verify convergence.

#include "pluss/codem/codem.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace pluss::subsp::math::roots {

using Fn = std::function<Series(const Series &)>;

struct Options final {
  std::int64_t max_iterations{100};
  double x_tolerance{0.0};   // stop when every lane's bracket / step is below this (0 = run all iterations)
  double f_tolerance{0.0};   // stop when every |f| is below this (0 = off)
  std::int64_t check_every{8}; // host check cadence for the early exits
  double derivative_step{1e-6}; // relative step for finite-difference Newton
};

struct Result final {
  Series root = pluss::series::undefined();
  Series residual = pluss::series::undefined(); // f(root)
  std::int64_t iterations{0};
  bool bracketed{true}; // false when some lane lacked a sign change (bracketed solvers)
};

namespace detail {
inline void require_same_shape(const Series &a, const Series &b, const char *who) {
  if (a.dim() != b.dim())
    throw std::invalid_argument(std::string("roots::") + who + ": lo and hi must have the same shape");
  for (std::int64_t i = 0; i < a.dim(); ++i)
    if (a.size(i) != b.size(i))
      throw std::invalid_argument(std::string("roots::") + who + ": lo and hi must have the same shape");
}
inline bool all_below(const Series &v, double tol) { return v.abs().max().item<double>() <= tol; }
} // namespace detail

inline Result bisect(const Fn &f, Series lo, Series hi, const Options &opt = {}) {
  detail::require_same_shape(lo, hi, "bisect");
  Series f_lo = f(lo);
  const Series f_hi = f(hi);
  Result out;
  out.bracketed = f_lo.mul(f_hi).le(0.0).all().item<bool>();
  for (std::int64_t i = 0; i < opt.max_iterations; ++i) {
    const Series mid = lo.add(hi).mul(0.5);
    const Series f_mid = f(mid);
    const Series same = f_mid.mul(f_lo).gt(0.0);
    lo = same.where(mid, lo);
    f_lo = same.where(f_mid, f_lo);
    hi = same.where(hi, mid);
    out.iterations = i + 1;
    if ((opt.x_tolerance > 0.0 || opt.f_tolerance > 0.0) && (i + 1) % opt.check_every == 0) {
      if ((opt.x_tolerance > 0.0 && detail::all_below(hi.sub(lo), opt.x_tolerance)) ||
          (opt.f_tolerance > 0.0 && detail::all_below(f(lo.add(hi).mul(0.5)), opt.f_tolerance)))
        break;
    }
  }
  out.root = lo.add(hi).mul(0.5);
  out.residual = f(out.root);
  return out;
}

// Regula falsi with the Illinois modification (Dowell & Jarratt 1971): the retained endpoint's
// function value is halved whenever the new iterate lands on the same side as the previous one,
// restoring superlinear convergence. The latest iterate `b` is always one bracket end.
inline Result illinois(const Fn &f, Series a, Series b, const Options &opt = {}) {
  detail::require_same_shape(a, b, "illinois");
  Series fa = f(a), fb = f(b);
  Result out;
  out.bracketed = fa.mul(fb).le(0.0).all().item<bool>();
  for (std::int64_t i = 0; i < opt.max_iterations; ++i) {
    const Series denom = fb.sub(fa);
    const Series safe = denom.abs().lt(1e-300).where(denom.mul(0.0).add(1.0), denom);
    const Series c = a.mul(fb).sub(b.mul(fa)).div(safe);
    const Series fc = f(c);
    const Series crossed = fc.mul(fb).lt(0.0); // root between b and c: the old b becomes the other end
    const Series a_new = crossed.where(b, a);
    const Series fa_new = crossed.where(fb, fa.mul(0.5)); // Illinois halving otherwise
    a = a_new;
    fa = fa_new;
    b = c;
    fb = fc;
    out.iterations = i + 1;
    if ((opt.x_tolerance > 0.0 || opt.f_tolerance > 0.0) && (i + 1) % opt.check_every == 0) {
      if ((opt.f_tolerance > 0.0 && detail::all_below(fb, opt.f_tolerance)) ||
          (opt.x_tolerance > 0.0 && detail::all_below(b.sub(a), opt.x_tolerance)))
        break;
    }
  }
  out.root = b;
  out.residual = fb;
  return out;
}

// Newton-Raphson. `df` may be empty: a central difference with a relative step is used.
inline Result newton(const Fn &f, Series x, const Fn &df = {}, const Options &opt = {}) {
  Result out;
  for (std::int64_t i = 0; i < opt.max_iterations; ++i) {
    const Series fx = f(x);
    Series slope = pluss::series::undefined();
    if (df) {
      slope = df(x);
    } else {
      const Series h = x.abs().clamp(1.0, 1e300).mul(opt.derivative_step);
      slope = f(x.add(h)).sub(f(x.sub(h))).div(h.mul(2.0));
    }
    const Series safe = slope.abs().lt(1e-300).where(slope.mul(0.0).add(1.0), slope);
    const Series step = fx.div(safe);
    x = x.sub(step);
    out.iterations = i + 1;
    if ((opt.x_tolerance > 0.0 || opt.f_tolerance > 0.0) && (i + 1) % opt.check_every == 0) {
      if ((opt.x_tolerance > 0.0 && detail::all_below(step, opt.x_tolerance)) ||
          (opt.f_tolerance > 0.0 && detail::all_below(fx, opt.f_tolerance)))
        break;
    }
  }
  out.root = x;
  out.residual = f(x);
  return out;
}

inline Result secant(const Fn &f, Series x0, Series x1, const Options &opt = {}) {
  Series f0 = f(x0), f1 = f(x1);
  Result out;
  for (std::int64_t i = 0; i < opt.max_iterations; ++i) {
    const Series denom = f1.sub(f0);
    const Series safe = denom.abs().lt(1e-300).where(denom.mul(0.0).add(1.0), denom);
    const Series x2 = x1.sub(f1.mul(x1.sub(x0)).div(safe));
    x0 = x1; f0 = f1;
    x1 = x2; f1 = f(x2);
    out.iterations = i + 1;
    if ((opt.x_tolerance > 0.0 || opt.f_tolerance > 0.0) && (i + 1) % opt.check_every == 0) {
      if ((opt.f_tolerance > 0.0 && detail::all_below(f1, opt.f_tolerance)) ||
          (opt.x_tolerance > 0.0 && detail::all_below(x1.sub(x0), opt.x_tolerance)))
        break;
    }
  }
  out.root = x1;
  out.residual = f1;
  return out;
}

} // namespace pluss::subsp::math::roots
