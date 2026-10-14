#pragma once

// Stiff ODE integration for autonomous systems dy/dt = f(y), batched over
// leading lanes. Port of the modified Rosenbrock (2,3) pair of Shampine &
// Reichelt, "The MATLAB ODE Suite", SIAM J. Sci. Comput. 18 (1997) 1-22, as
// implemented in MATLAB R2025b `toolbox/matlab/funfun/ode23s.m` (read from the
// local install; constants d = 1/(2+sqrt 2), e32 = 6 + sqrt 2; step control
// err = (h/6)||(k1 - 2 k2 + k3)/max(|y|,|ynew|,threshold)||_inf,
// threshold = AbsTol/RelTol as in matlab.ode.internal.DESolver, step factor
// 0.8 (rtol/err)^(1/3) clamped to [0.1, ..] on failure and 1.25 (err/rtol)^(1/3)
// / x5 growth on success; initial step from the same two-stage estimate).
//
// Differences from MATLAB: the system must be autonomous (the df/dt term of
// ode23s is identically zero), all lanes share one step (the error is the max
// over lanes, which is conservative), and the Jacobian defaults to scaled
// central differences evaluated for all lanes and columns in a single call.

#include "pluss/subsp/natsci/common/dynamics.h"

#include <algorithm>
#include <limits>

namespace pluss::subsp::natsci::common::stiff {

using dynamics::RateFn;

struct Options final {
  double rtol{1e-6};
  double atol{1e-10};
  std::int64_t max_steps{200000};
  double h_initial{0.0}; // 0 = automatic (MATLAB's estimate)
  double h_max{std::numeric_limits<double>::infinity()};
  double fd_relative_step{1.5e-8}; // sqrt(machine epsilon)
};

struct Result final {
  Series y = pluss::series::undefined();
  double t{0.0};
  std::int64_t steps{0};
  std::int64_t rejected{0};
  std::int64_t rhs_evaluations{0};
  std::int64_t jacobian_evaluations{0};
  bool converged{false};
};

// Optional analytic Jacobian: y [..., n] -> [..., n, n].
using JacobianFn = std::function<Series(const Series &)>;

namespace detail {
inline double inf_norm(const Series &v) { return v.abs().max().item<double>(); }
} // namespace detail

inline Result integrate(const RateFn &f, const Series &y0, double t0, double t1,
                        const Options &opt = {}, const JacobianFn &jac = {},
                        double *step_hint = nullptr) {
  dynamics::require(static_cast<bool>(f), "stiff: rate callback is required");
  dynamics::require(t1 >= t0 && std::isfinite(t0) && std::isfinite(t1),
                    "stiff: need finite t0 <= t1");
  dynamics::require(opt.rtol > 0.0 && opt.atol > 0.0, "stiff: tolerances must be positive");
  dynamics::require_finite(y0, "stiff: initial state");
  const double threshold = opt.atol / opt.rtol;
  const double d = 1.0 / (2.0 + std::sqrt(2.0));
  const double e32 = 6.0 + std::sqrt(2.0);
  const double pow3 = 1.0 / 3.0;
  const std::int64_t n = y0.size(-1);
  const Config cfg = Config{}.with_dtype(y0.scalar_type()).with_platform(y0.platform());
  const Series eye = pluss::series::eye(n, cfg);

  Result out;
  out.y = y0;
  out.t = t0;
  out.converged = true;
  if (t1 == t0)
    return out;

  const auto jacobian_at = [&](const Series &y) {
    ++out.jacobian_evaluations;
    if (jac)
      return jac(y);
    const Series step = y.abs().clamp(threshold, 1e300).mul(opt.fd_relative_step);
    out.rhs_evaluations += 2 * n;
    return dynamics::jacobian_scaled(f, y, step);
  };
  const auto factorize = [&](const Series &j, double h) {
    return pluss::subsp::linearalg::inverse(eye.sub(j.mul(h * d)));
  };
  const auto apply = [&](const Series &winv, const Series &rhs) {
    return winv.matmul(rhs.unsqueeze(-1)).squeeze(-1);
  };
  const auto rhs_eval = [&](const Series &y) {
    ++out.rhs_evaluations;
    return f(y);
  };

  const double span = t1 - t0;
  const double hmax = std::min(opt.h_max, span);
  double t = t0;
  Series y = y0;
  Series f0 = rhs_eval(y);
  Series j = jacobian_at(y);
  const double hmin = 16.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(t0), std::abs(t1));

  double absh = 0.0;
  if (step_hint && *step_hint > 0.0) {
    absh = std::min(hmax, std::max(hmin, *step_hint));
  } else if (opt.h_initial > 0.0) {
    absh = std::min(hmax, std::max(hmin, opt.h_initial));
  } else {
    // MATLAB's two-stage initial step estimate (autonomous: dfdt = 0).
    const Series wt = y.abs().clamp(threshold, 1e300);
    double rh = 1.25 * detail::inf_norm(f0.div(wt)) / std::pow(opt.rtol, pow3);
    absh = hmax;
    if (absh * rh > 1.0)
      absh = 1.0 / rh;
    absh = std::max(absh, hmin);
    const Series dfdt = apply(j, f0); // df/dy * f0
    const Series solved = apply(factorize(j, absh), dfdt);
    rh = 1.25 * std::sqrt(0.5 * detail::inf_norm(solved.div(wt))) / std::pow(opt.rtol, pow3);
    absh = hmax;
    if (absh * rh > 1.0)
      absh = 1.0 / rh;
    absh = std::max(absh, hmin);
  }

  bool done = false;
  while (!done) {
    if (out.steps + out.rejected >= opt.max_steps) {
      out.converged = false;
      break;
    }
    absh = std::min(hmax, std::max(hmin, absh));
    double h = absh;
    if (1.1 * absh >= t1 - t) {
      h = t1 - t;
      absh = h;
      done = true;
    }
    bool nofailed = true;
    double err = 0.0;
    Series ynew = y;
    while (true) {
      const Series winv = factorize(j, h);
      const Series k1 = apply(winv, f0);
      const Series f1 = rhs_eval(y.add(k1.mul(0.5 * h)));
      const Series k2 = apply(winv, f1.sub(k1)).add(k1);
      ynew = y.add(k2.mul(h));
      const Series f2 = rhs_eval(ynew);
      const Series k3 = apply(winv, f2.sub(k2.sub(f1).mul(e32)).sub(k1.sub(f0).mul(2.0)));
      const Series scale = y.abs().maximum(ynew.abs()).clamp(threshold, 1e300);
      err = (absh / 6.0) * detail::inf_norm(k1.sub(k2.mul(2.0)).add(k3).div(scale));
      if (!std::isfinite(err))
        err = std::numeric_limits<double>::infinity();
      if (err > opt.rtol) {
        ++out.rejected;
        if (absh <= hmin || out.steps + out.rejected >= opt.max_steps) {
          out.converged = false;
          done = true;
          break;
        }
        const double factor = std::isfinite(err) ? std::max(0.1, 0.8 * std::pow(opt.rtol / err, pow3)) : 0.1;
        absh = std::max(hmin, absh * factor);
        h = absh;
        done = false;
        nofailed = false;
        if (1.1 * absh >= t1 - t) {
          h = t1 - t;
          absh = h;
          done = true;
        }
        continue;
      }
      break;
    }
    if (!out.converged)
      break;
    ++out.steps;
    t = done ? t1 : t + h;
    y = ynew;
    if (done)
      break;
    if (nofailed) {
      const double temp = 1.25 * std::pow(err / opt.rtol, pow3);
      absh = temp > 0.2 ? absh / temp : 5.0 * absh;
    }
    f0 = rhs_eval(y);
    j = jacobian_at(y);
  }
  out.y = y;
  out.t = t;
  if (step_hint)
    *step_hint = absh;
  return out;
}

// States at a list of increasing output times (first entry = t0 state is
// y0 itself): returns [len(times), ..., n]; step size carries across intervals.
inline Series integrate_at(const RateFn &f, const Series &y0, const std::vector<double> &times,
                           const Options &opt = {}, const JacobianFn &jac = {}) {
  dynamics::require(times.size() >= 2, "stiff: need at least two output times");
  std::vector<Series> frames{y0.unsqueeze(0)};
  Series y = y0;
  double hint = opt.h_initial;
  for (std::size_t i = 1; i < times.size(); ++i) {
    const auto r = integrate(f, y, times[i - 1], times[i], opt, jac, &hint);
    dynamics::require(r.converged, "stiff: step budget exhausted or step size underflow");
    y = r.y;
    frames.push_back(y.unsqueeze(0));
  }
  Series out = frames.front();
  if (frames.size() > 1)
    out = out.unite(std::vector<Series>(frames.begin() + 1, frames.end()), 0);
  return out;
}

} // namespace pluss::subsp::natsci::common::stiff
