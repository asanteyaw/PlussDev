#pragma once

// Nonlinear least squares with confidence intervals and profile likelihood, batched over independent fits (lanes).
//
//  * Levenberg-Marquardt exactly as MATLAB's nlinfit (Statistics Toolbox R2025b, `LMfit` in toolbox/stats/stats/nlinfit.m, read from the
//    local install): forward-difference Jacobian with relative step DerivStep (eps^(1/3), `statjacobian`: delta = DerivStep * theta, or
//    DerivStep * ||theta|| (1 if zero) for a zero parameter), damping lambda0 = 0.01 with Marquardt scaling diag(J^T J),
//    step = (J^T J + lambda diag(J^T J))^{-1} J^T r, lambda <- max(0.1 lambda, eps) after a decreasing step, lambda <- 10 lambda until the
//    sum of squares decreases (stall when lambda > 1e16); stop on ||step|| < TolX (sqrt(eps) + ||beta||) or |dSSE| <= TolFun SSE
//    (TolX = TolFun = 1e-8).
//  * Parameter intervals as nlparci: beta +- t_{1-alpha/2, n-p} rmse sqrt(diag((J^T J)^{-1})), rmse = ||r|| / sqrt(n - p).
//  * Profile likelihood (Raue et al., Bioinformatics 25 (2009) 1923, as implemented in Data2Dynamics `PLE/arPLEInit.m`: threshold
//    chi2inv(1 - alpha, 1) for pointwise intervals, chi2inv(1 - alpha, p) simultaneously): for a grid of values of one parameter the others
//    are re-fitted; with Gaussian errors of standard deviation sigma the profile is chi2(theta_i) = SSE_profile(theta_i)/sigma^2 and the
//    interval is {chi2 - chi2_min <= threshold}. Flat profiles reveal structurally or practically non-identifiable parameters.
//  * Critical values come from bisection on the library's distribution CDFs (the distribution objects have no quantile function).

#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/common/dynamics.h"
#include "pluss/subsp/stats/distr.h"

#include <cmath>
#include <functional>
#include <limits>
#include <vector>

namespace pluss::subsp::natsci::common::estimation {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace roots = ::pluss::subsp::math::roots;

using ModelFn = std::function<Series(const Series &beta)>; // beta [L, P] -> predictions [L, M]

struct Options final {
  double tol_x = 1e-8;
  double tol_fun = 1e-8;
  double derivative_step = 6.0554544523933429e-06; // eps^(1/3), nlinfit's default
  int max_iterations = 100;
};

struct Fit final {
  Series beta = pluss::series::undefined();       // [L, P]
  Series sse = pluss::series::undefined();        // [L]
  Series residuals = pluss::series::undefined();  // [L, M]  (y - model)
  Series jacobian = pluss::series::undefined();   // [L, M, P] at beta
  Series iterations = pluss::series::undefined(); // [L]
  Series converged = pluss::series::undefined();  // [L] 1 when a tolerance, not the iteration cap or a stall, ended the lane
  std::int64_t observations{0}, parameters{0};
};

namespace detail {
// forward-difference Jacobian [L, M, P] and the step used, as statjacobian
inline Series jacobian(const ModelFn &model, const Series &beta, const Series &yfit, double step) {
  const std::int64_t l = beta.size(0), p = beta.size(1), m = yfit.size(1);
  const Series norm = beta.square().sum(-1, true).sqrt();                                          // [L, 1]
  const Series base = beta.abs().gt(0.0).where(beta.mul(step), norm.gt(0.0).where(norm, norm.mul(0.0).add(1.0)).mul(step)); // delta [L, P]
  const Config cfg = Config{}.with_dtype(beta.scalar_type()).with_platform(beta.platform());
  const Series eye = pluss::series::eye(p, cfg);                                                    // [P, P]
  const Series perturbed = beta.unsqueeze(1).add(base.unsqueeze(-1).mul(eye.unsqueeze(0))); // [L, P, P]: row i of lane l = beta_l + delta_{l,i} e_i
  const Series flat = perturbed.reshape({l * p, p});
  const Series yplus = model(flat).reshape({l, p, m});                                              // [L, P, M]
  return yplus.sub(yfit.unsqueeze(1)).div(base.unsqueeze(-1)).transpose(1, 2);                      // [L, M, P]
}
inline Series sse_of(const Series &r) { return r.square().sum(-1, false); }
inline double sqrt_eps() { return std::sqrt(std::numeric_limits<double>::epsilon()); }
} // namespace detail

// Levenberg-Marquardt for L independent fits of the same model to data y [M] (shared) or [L, M]
inline Fit levenberg_marquardt(const ModelFn &model, const Series &y, const Series &beta0, const Options &opt = {}) {
  dynamics::require(beta0.dim() == 2, "estimation: beta0 must be [lanes, parameters]");
  const std::int64_t l = beta0.size(0), p = beta0.size(1);
  const Series data = y.dim() == 1 ? y.unsqueeze(0).expand({l, y.size(0)}) : y;
  const std::int64_t m = data.size(1);
  Series beta = beta0;
  Series yfit = model(beta);
  Series r = data.sub(yfit);
  Series sse = detail::sse_of(r);
  Series lambda = sse.mul(0.0).add(0.01);
  Series active = sse.mul(0.0).add(1.0); // 1 while the lane is still iterating
  Series iters = sse.mul(0.0);
  Series converged = sse.mul(0.0);
  const double eps = std::numeric_limits<double>::epsilon();
  const Config cfg = Config{}.with_dtype(beta.scalar_type()).with_platform(beta.platform());
  const Series eye = pluss::series::eye(p, cfg);
  Series jac = detail::jacobian(model, beta, yfit, opt.derivative_step);
  for (int it = 0; it < opt.max_iterations && active.sum().item<double>() > 0.0; ++it) {
    jac = detail::jacobian(model, beta, yfit, opt.derivative_step);
    const Series jt = jac.transpose(1, 2);                                  // [L, P, M]
    const Series jtj = jt.matmul(jac);                                      // [L, P, P]
    const Series diag = jtj.mul(eye).sum(-1, false).clamp(1e-300, 1e300);   // [L, P] (floor keeps a zero column solvable)
    const Series jtr = jt.matmul(r.unsqueeze(-1)).squeeze(-1);              // [L, P]
    const Series beta_old = beta, sse_old = sse;
    // trial steps with the current damping; failed lanes raise lambda and retry (batched, at most 40 retries)
    Series trial_beta = beta, trial_sse = sse, trial_r = r, trial_step = beta.mul(0.0);
    Series pending = active; // lanes still searching for a decreasing step
    Series lam = lambda;
    Series step_final = beta.mul(0.0);
    Series done = active.mul(0.0);
    for (int inner = 0; inner < 40; ++inner) {
      const Series damped = jtj.add(eye.unsqueeze(0).mul(diag.unsqueeze(-1)).mul(lam.unsqueeze(-1).unsqueeze(-1)));
      const Series step = dynamics::solve(damped, jtr);
      const Series nb = beta_old.add(step);
      const Series ny = model(nb);
      const Series nr = data.sub(ny);
      const Series ns = detail::sse_of(nr);
      const Series finite = ns.sub(ns).eq(0.0); // not NaN / inf
      const Series improved = ns.lt(sse_old).logical_and(finite);
      const Series take = improved.logical_and(done.eq(0.0)).logical_and(pending.gt(0.0));
      trial_beta = take.unsqueeze(-1).where(nb, trial_beta);
      trial_sse = take.where(ns, trial_sse);
      trial_r = take.unsqueeze(-1).where(nr, trial_r);
      step_final = take.unsqueeze(-1).where(step, step_final);
      done = take.where(done.mul(0.0).add(1.0), done);
      // the first trial of a lane that did not improve: raise lambda (the first inner pass uses the standing lambda)
      const Series fail = improved.logical_not().logical_and(done.eq(0.0)).logical_and(pending.gt(0.0));
      lam = fail.where(lam.mul(10.0), lam);
      if (done.mul(pending).sum().item<double>() >= pending.sum().item<double>())
        break;
    }
    const Series stalled = pending.gt(0.0).logical_and(done.eq(0.0)); // lambda ran past 1e16 without a decrease
    const Series ok = pending.gt(0.0).logical_and(done.gt(0.0));
    beta = ok.unsqueeze(-1).where(trial_beta, beta);
    r = ok.unsqueeze(-1).where(trial_r, r);
    sse = ok.where(trial_sse, sse);
    yfit = data.sub(r);
    // lambda bookkeeping: after a success at the standing lambda it shrinks; after retries it keeps the raised value
    const Series first_try = lam.sub(lambda).abs().lt(1e-300);
    lambda = ok.where(first_try.where(lambda.mul(0.1).clamp(eps, 1e300), lam), lambda);
    iters = iters.add(active);
    const Series step_norm = step_final.square().sum(-1, false).sqrt();
    const Series beta_norm = beta.square().sum(-1, false).sqrt();
    const Series tolx = step_norm.lt(beta_norm.add(detail::sqrt_eps()).mul(opt.tol_x)).logical_and(ok);
    const Series tolfun = sse.sub(sse_old).abs().le(sse.mul(opt.tol_fun)).logical_and(ok);
    const Series stop = tolx.logical_or(tolfun).logical_or(stalled);
    converged = tolx.logical_or(tolfun).logical_and(active.gt(0.0)).where(converged.mul(0.0).add(1.0), converged);
    active = stop.where(active.mul(0.0), active);
  }
  Fit out;
  out.beta = beta;
  out.sse = sse;
  out.residuals = r;
  out.jacobian = detail::jacobian(model, beta, yfit, opt.derivative_step);
  out.iterations = iters;
  out.converged = converged;
  out.observations = m;
  out.parameters = p;
  return out;
}

// ---- critical values -----------------------------------------------------------------------------------------------------
// Student t quantile for probability q and degrees of freedom nu by bisection on the CDF
inline double student_t_quantile(double q, double nu) {
  const pluss::subsp::stats::distr::StudentT t{nu, 0.0, 1.0};
  const auto cdf = [&](const Series &x) { return t.cdf(x).sub(q); };
  const Series lo = pluss::make_series(std::vector<double>{-1e3}, {1}).to(pluss::ScalarType::Float64), hi = pluss::make_series(std::vector<double>{1e3}, {1}).to(pluss::ScalarType::Float64);
  roots::Options opt;
  opt.max_iterations = 200;
  return roots::bisect(cdf, lo, hi, opt).root.item<double>();
}
// chi-square quantile (integer degrees of freedom) by bisection on the CDF
inline double chi2_quantile(double q, std::int64_t dof) {
  const pluss::subsp::stats::distr::ChiSquare c{dof};
  const auto cdf = [&](const Series &x) { return c.cdf(x).sub(q); };
  const Series lo = pluss::make_series(std::vector<double>{1e-12}, {1}).to(pluss::ScalarType::Float64), hi = pluss::make_series(std::vector<double>{1e3}, {1}).to(pluss::ScalarType::Float64);
  roots::Options opt;
  opt.max_iterations = 200;
  return roots::bisect(cdf, lo, hi, opt).root.item<double>();
}

// ---- intervals ---------------------------------------------------------------------------------------------------------------
struct Intervals final {
  Series lower = pluss::series::undefined(); // [L, P]
  Series upper = pluss::series::undefined();
  Series standard_error = pluss::series::undefined();
};
// nlparci: beta +- t_{1-alpha/2, n-p} rmse sqrt(diag((J^T J)^{-1}))
inline Intervals confidence_intervals(const Fit &fit, double alpha = 0.05) {
  const std::int64_t n = fit.observations, p = fit.parameters, nu = n - p;
  dynamics::require(nu > 0, "estimation: need more observations than parameters");
  const Series jtj = fit.jacobian.transpose(1, 2).matmul(fit.jacobian);
  const Series inv = pluss::subsp::linearalg::inverse(jtj);
  const Config cfg = Config{}.with_dtype(fit.beta.scalar_type()).with_platform(fit.beta.platform());
  const Series diag = inv.mul(pluss::series::eye(p, cfg)).sum(-1, false);
  const Series rmse = fit.sse.div(static_cast<double>(nu)).sqrt();
  const Series se = diag.sqrt().mul(rmse.unsqueeze(-1));
  const double t = student_t_quantile(1.0 - alpha / 2.0, static_cast<double>(nu));
  Intervals out;
  out.standard_error = se;
  out.lower = fit.beta.sub(se.mul(t));
  out.upper = fit.beta.add(se.mul(t));
  return out;
}

// ---- profile likelihood --------------------------------------------------------------------------------------------------------
struct Profile final {
  Series values = pluss::series::undefined(); // [G] grid of the profiled parameter
  Series sse = pluss::series::undefined();    // [G] re-optimised sum of squares
  Series beta = pluss::series::undefined();   // [G, P] re-fitted parameters (the profiled one equals `values`)
};
// profile of parameter `index` over `grid` [G]: the other parameters are re-fitted from `start` [P] for every grid value (all grid
// points in one batched Levenberg-Marquardt call). The profiled coordinate is held fixed by zeroing its Jacobian column and step.
inline Profile profile_likelihood(const ModelFn &model, const Series &y, const Series &start, std::int64_t index, const Series &grid, const Options &opt = {}) {
  const std::int64_t g = grid.size(0), p = start.size(0);
  std::vector<std::int64_t> keep;
  for (std::int64_t i = 0; i < p; ++i)
    if (i != index)
      keep.push_back(i);
  dynamics::require(!keep.empty(), "profile_likelihood: need at least two parameters");
  const auto assemble = [&](const Series &red, const Series &values) { // [lanes, P-1] + [lanes] -> [lanes, P]
    std::vector<Series> cols;
    std::int64_t k = 0;
    for (std::int64_t i = 0; i < p; ++i)
      cols.push_back(i == index ? values.unsqueeze(-1) : red.select(-1, k++).unsqueeze(-1));
    return cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), -1);
  };
  // the model sees full parameter vectors; lanes of the Jacobian sweep repeat the grid values, so tile `grid` to the lane count
  const ModelFn wrapped = [&](const Series &red) {
    const std::int64_t reps = red.size(0) / g;
    Series values = grid;
    if (reps > 1) // the Jacobian sweep lists each lane `reps` times in a row
      values = grid.unsqueeze(1).expand({g, reps}).reshape({g * reps});
    return model(assemble(red, values));
  };
  std::vector<Series> start_cols;
  for (const auto i : keep)
    start_cols.push_back(start.select(0, i).unsqueeze(0).expand({g}).unsqueeze(-1));
  const Series red0 = start_cols.front().unite(std::vector<Series>(start_cols.begin() + 1, start_cols.end()), -1);
  const Fit fit = levenberg_marquardt(wrapped, y, red0, opt);
  Profile out;
  out.values = grid;
  out.sse = fit.sse;
  out.beta = assemble(fit.beta, grid);
  return out;
}
// the interval {theta : (SSE_profile - SSE_min)/sigma^2 <= chi2inv(1 - alpha, dof)} read off a profile by linear interpolation
// between grid points; dof = 1 for pointwise, p for simultaneous intervals. Returns {lower, upper} (NaN when the profile does not reach the threshold)
inline std::pair<double, double> profile_interval(const Profile &profile, double sse_min, double sigma2, double alpha, std::int64_t dof) {
  const double threshold = chi2_quantile(1.0 - alpha, dof);
  const auto v = profile.values.to_vec<double>();
  const auto s = profile.sse.to_vec<double>();
  double lo = std::numeric_limits<double>::quiet_NaN(), hi = lo;
  const auto crossing = [&](std::size_t a, std::size_t b) {
    const double fa = (s[a] - sse_min) / sigma2 - threshold, fb = (s[b] - sse_min) / sigma2 - threshold;
    return v[a] + (v[b] - v[a]) * fa / (fa - fb);
  };
  for (std::size_t i = 0; i + 1 < v.size(); ++i) {
    const double fa = (s[i] - sse_min) / sigma2 - threshold, fb = (s[i + 1] - sse_min) / sigma2 - threshold;
    if (fa > 0.0 && fb <= 0.0)
      lo = crossing(i, i + 1);
    if (fa <= 0.0 && fb > 0.0)
      hi = crossing(i, i + 1);
  }
  return {lo, hi};
}

} // namespace pluss::subsp::natsci::common::estimation
