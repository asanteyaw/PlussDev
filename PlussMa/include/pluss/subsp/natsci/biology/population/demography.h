#pragma once

// Demography on top of structured.h (matrix models): everything that needs more than the dominant eigenpair of one matrix.
//
//  * Stage-structured quantities (Caswell, Matrix Population Models; definitions as implemented in the R package popbio, CRAN mirror
//    github.com/cran/popbio, files fundamental.matrix.R, net.reproductive.rate.R, generation.time.R, damping.ratio.R, LTRE.R,
//    stoch.growth.rate.R, read): fundamental matrix N = (I - T)^-1 of the transition part, its variances, expected lifetime, net
//    reproductive rate R0 = rho(F N), generation time ln R0 / ln lambda, damping ratio lambda_1 / |lambda_2|, fixed-design LTRE.
//  * Environmental stochasticity: Tuljapurkar's small-noise approximation ln lambda_s ~ ln lambda(Abar) - tau^2 / (2 lambda^2) with
//    tau^2 = sum_k p_k <S, A_k - Abar>^2, and exact-in-the-limit simulation of ln lambda_s for i.i.d. or Markov environments, run on
//    many replicates in parallel lanes; quasi-extinction probability from the simulated paths.
//  * Life tables: Euler-Lotka growth rate by Newton, cohort generation time, stable age distribution and Fisher's reproductive value.
//  * Transient dynamics: first-step and maximum amplification of the standardised matrix (Neubert & Caswell 1997; Stott et al. 2011
//    definitions via the 2-norm), reactivity.
//  * Sensitivity of lambda to lower-level parameters through the chain rule  d lambda / d theta = sum_ij s_ij d a_ij / d theta.
//
// All matrices are [..., k, k]; leading axes are independent lanes.

#include "pluss/subsp/linearalg/linearalg.h"
#include "pluss/subsp/natsci/biology/population/structured.h"
#include "pluss/tils/auxiliary/rng_seed.h"

#include <cmath>

namespace pluss::subsp::natsci::biology::population::demography {

namespace structured = ::pluss::subsp::natsci::biology::population::structured;
using structured::Eigen;

namespace detail {
inline Series eye_like(const Series &m) {
  const Config cfg = Config{}.with_dtype(m.scalar_type()).with_platform(m.platform());
  return pluss::series::eye(m.size(-1), cfg);
}
inline Series frobenius(const Series &m) { return m.square().sum(-1, false).sum(-1, false).sqrt(); }
inline Series diagonal(const Series &m) { return m.mul(eye_like(m)).sum(-1, false); }
} // namespace detail

// ---- stage-structured quantities ----------------------------------------------------------------------------------------------
// popbio's default split: fertilities are the first row except its first column (a clonal/stasis entry stays a transition)
inline Series first_row_fertility_mask(std::int64_t k, const Series &like) {
  std::vector<double> m(static_cast<std::size_t>(k * k), 0.0);
  for (std::int64_t j = 1; j < k; ++j)
    m[static_cast<std::size_t>(j)] = 1.0;
  return pluss::make_series(m, {k, k}).to(like.scalar_type()).to(like.platform());
}

struct Fundamental final {
  Series n = pluss::series::undefined();               // (I - T)^-1: expected time in stage i for an individual starting in j
  Series variance = pluss::series::undefined();        // variance of that time
  Series expected_lifetime = pluss::series::undefined(); // column sums of N
  Series lifetime_variance = pluss::series::undefined();
};
inline Fundamental fundamental_matrix(const Series &transitions) {
  const Series n = pluss::subsp::linearalg::inverse(detail::eye_like(transitions).sub(transitions));
  Fundamental out;
  out.n = n;
  out.variance = detail::diagonal(n).mul(2.0).sub(1.0).unsqueeze(-1).mul(n).sub(n.square());
  out.expected_lifetime = n.sum(-2, false);
  const Series two = n.matmul(n).mul(2.0).sub(n).sum(-2, false);
  out.lifetime_variance = two.sub(out.expected_lifetime.square());
  return out;
}

// R0 = dominant eigenvalue of F N (mean lifetime offspring production per generation)
inline Series net_reproductive_rate(const Series &matrix, const Series &fertility_mask) {
  const Series f = matrix.mul(fertility_mask), t = matrix.sub(f);
  const Series r = f.matmul(fundamental_matrix(t).n);
  return structured::dominant(r).lambda;
}
inline Series generation_time(const Series &matrix, const Series &fertility_mask) {
  return net_reproductive_rate(matrix, fertility_mask).log().div(structured::dominant(matrix).lambda.log());
}

// Spectral radius by Gelfand's formula rho = lim ||B^(2^m)||^(1/2^m), computed with repeated squaring and renormalisation (stable for
// complex-conjugate and defective cases, all lanes at once).
inline Series spectral_radius(const Series &b, int squarings = 40) {
  Series r = b, c = b.sum(-1, false).sum(-1, false).mul(0.0);
  Series s = detail::frobenius(r).clamp(1e-300, 1e300);
  r = r.div(s.unsqueeze(-1).unsqueeze(-1));
  c = s.log();
  for (int i = 0; i < squarings; ++i) {
    r = r.matmul(r);
    s = detail::frobenius(r).clamp(1e-300, 1e300);
    r = r.div(s.unsqueeze(-1).unsqueeze(-1));
    c = c.mul(2.0).add(s.log());
  }
  return c.div(std::ldexp(1.0, squarings)).exp();
}

// lambda_1 / |lambda_2| by Hotelling deflation of the dominant pair; equals 1 for imprimitive matrices (tied moduli)
inline Series damping_ratio(const Series &matrix) {
  const Eigen e = structured::dominant(matrix);
  const Series b = matrix.sub(e.right.unsqueeze(-1).mul(e.left.unsqueeze(-2)).mul(e.lambda.unsqueeze(-1).unsqueeze(-1)));
  return e.lambda.div(spectral_radius(b));
}

// Fixed-design LTRE: contributions (A_trt - A_ref) .* S((A_trt + A_ref) / 2), summing to ~ lambda_trt - lambda_ref
inline Series ltre_fixed(const Series &treatment, const Series &reference) {
  const Series mid = treatment.add(reference).mul(0.5);
  return treatment.sub(reference).mul(structured::sensitivity(structured::dominant(mid)));
}

// d lambda / d theta_p for a_ij(theta) given by `matrix_fn(theta) -> [..., k, k]`; theta [..., P]; returns [..., P]
inline Series lambda_gradient(const std::function<Series(const Series &)> &matrix_fn, const Series &theta, double relative_step = 1e-6) {
  const Series a0 = matrix_fn(theta);
  const std::int64_t k = a0.size(-1);
  const auto flat = [&](const Series &t) {
    const Series a = matrix_fn(t);
    std::vector<std::int64_t> shape;
    for (std::int64_t i = 0; i + 2 < a.dim(); ++i)
      shape.push_back(a.size(i));
    shape.push_back(k * k);
    return a.reshape(shape);
  };
  const Series step = theta.abs().clamp(1e-8, 1e300).mul(relative_step);
  const Series dadt = dynamics::jacobian_scaled(flat, theta, step); // [..., k*k, P]
  std::vector<std::int64_t> shape;
  for (std::int64_t i = 0; i + 2 < a0.dim(); ++i)
    shape.push_back(a0.size(i));
  shape.push_back(k * k);
  shape.push_back(1);
  const Series s = structured::sensitivity(structured::dominant(a0)).reshape(shape);
  return dadt.mul(s).sum(-2, false);
}

// ---- transient dynamics -------------------------------------------------------------------------------------------------------
// sigma_max(A^t / lambda^t): the largest factor by which an (L2-normalised) population vector can exceed its asymptotic growth at step t.
// t = 1 is the first-step amplification (reactivity + 1); the maximum over t <= horizon is the maximum amplification.
struct Amplification final {
  Series first_step = pluss::series::undefined(); // [...]
  Series maximum = pluss::series::undefined();    // [...]
  Series time_of_maximum = pluss::series::undefined();
};
inline Amplification amplification(const Series &matrix, std::int64_t horizon) {
  dynamics::require(horizon >= 1, "amplification: horizon must be positive");
  const Series lam = structured::dominant(matrix).lambda;
  const Series a_hat = matrix.div(lam.unsqueeze(-1).unsqueeze(-1));
  Series power = a_hat;
  Amplification out;
  out.first_step = pluss::subsp::linearalg::svd_s(a_hat, false).amax(-1, false);
  out.maximum = out.first_step;
  out.time_of_maximum = out.first_step.mul(0.0).add(1.0);
  for (std::int64_t t = 2; t <= horizon; ++t) {
    power = power.matmul(a_hat);
    const Series s = pluss::subsp::linearalg::svd_s(power, false).amax(-1, false);
    const Series better = s.gt(out.maximum);
    out.time_of_maximum = better.where(out.time_of_maximum.mul(0.0).add(static_cast<double>(t)), out.time_of_maximum);
    out.maximum = better.where(s, out.maximum);
  }
  return out;
}

// ---- life tables --------------------------------------------------------------------------------------------------------------
// survivorship l [..., X] and fecundity m [..., X] at the given ages (default 0, 1, ..., X-1)
struct LifeTable final {
  Series r0 = pluss::series::undefined();               // sum l m
  Series cohort_generation_time = pluss::series::undefined(); // sum x l m / R0
  Series r = pluss::series::undefined();                // intrinsic rate: sum l m exp(-r x) = 1
  Series lambda = pluss::series::undefined();           // exp(r)
  Series stable_age = pluss::series::undefined();       // exp(-r x) l, normalised to sum 1
  Series reproductive_value = pluss::series::undefined(); // Fisher: exp(r x) / l_x sum_{y >= x} exp(-r y) l_y m_y
};
// `weights` are quadrature weights per age (default 1, discrete ages); with a midpoint grid of width h pass h to approximate the
// continuous Euler-Lotka equation  \int l(a) m(a) e^{-r a} da = 1.
inline LifeTable life_table(const Series &l, const Series &m, const Series &ages = pluss::series::undefined(), std::int64_t newton_steps = 60,
                            const Series &weights = pluss::series::undefined()) {
  dynamics::require(l.size(-1) == m.size(-1), "life_table: l and m must have the same number of ages");
  const std::int64_t x = l.size(-1);
  const Config cfg = Config{}.with_dtype(l.scalar_type()).with_platform(l.platform());
  const Series age = ages.defined() ? ages.to(l.scalar_type()).to(l.platform()) : pluss::series::arange(0, x, 1, l.scalar_type()).to(l.platform());
  const Series lm = weights.defined() ? l.mul(m).mul(weights.to(l.scalar_type()).to(l.platform())) : l.mul(m);
  LifeTable out;
  out.r0 = lm.sum(-1, false);
  out.cohort_generation_time = lm.mul(age).sum(-1, false).div(out.r0);
  Series r = out.r0.log().div(out.cohort_generation_time);
  for (std::int64_t i = 0; i < newton_steps; ++i) { // h(r) = sum lm e^{-r x} - 1 is convex and decreasing
    const Series w = lm.mul(age.mul(r.unsqueeze(-1)).neg().exp());
    const Series h = w.sum(-1, false).sub(1.0), dh = w.mul(age).sum(-1, false).neg();
    r = r.sub(h.div(dh));
  }
  out.r = r;
  out.lambda = r.exp();
  const Series disc = age.mul(r.unsqueeze(-1)).neg().exp();
  const Series c = weights.defined() ? disc.mul(l).mul(weights.to(l.scalar_type()).to(l.platform())) : disc.mul(l);
  out.stable_age = c.div(c.sum(-1, true));
  const Series tail = disc.mul(lm).flip(-1).cumsum(-1, false).flip(-1); // sum_{y >= x}
  out.reproductive_value = disc.reciprocal().mul(tail).div(l);
  return out;
}

// ---- environmental stochasticity -----------------------------------------------------------------------------------------------
struct TuljapurkarApproximation final {
  Series log_lambda_s = pluss::series::undefined(); // ln lambda(Abar) - tau^2 / (2 lambda^2)
  Series log_lambda_mean = pluss::series::undefined();
  Series tau2 = pluss::series::undefined();
};
// matrices [..., K, k, k], prob [..., K] or [K]. `popbio_covariance` rescales the covariance by K / (K - 1) as popbio's stoch.growth.rate does.
inline TuljapurkarApproximation tuljapurkar(const Series &matrices, const Series &prob, bool popbio_covariance = false) {
  const Series p = prob.unsqueeze(-1).unsqueeze(-1);
  const Series mean = matrices.mul(p).sum(-3, false);
  const Eigen e = structured::dominant(mean);
  const Series s = structured::sensitivity(e);
  const Series proj = matrices.sub(mean.unsqueeze(-3)).mul(s.unsqueeze(-3)).sum(-1, false).sum(-1, false); // <S, A_k - Abar>  [..., K]
  Series tau2 = proj.square().mul(prob).sum(-1, false);
  if (popbio_covariance) {
    const double kk = static_cast<double>(matrices.size(-3));
    tau2 = tau2.mul(kk / (kk - 1.0));
  }
  TuljapurkarApproximation out;
  out.tau2 = tau2;
  out.log_lambda_mean = e.lambda.log();
  out.log_lambda_s = out.log_lambda_mean.sub(tau2.div(e.lambda.square().mul(2.0)));
  return out;
}

struct StochasticGrowth final {
  Series per_replicate = pluss::series::undefined(); // mean ln growth per replicate [R]
  double mean{0.0};
  double standard_error{0.0};                        // from the spread of replicate means
};
struct Environment final {
  Series matrices = pluss::series::undefined(); // [K, k, k]
  Series prob = pluss::series::undefined();     // [K] (i.i.d. environments) - or the stationary start of a Markov chain
  Series transition = pluss::series::undefined(); // optional [K, K] row-stochastic environment chain
};

namespace detail {
// one environment index per replicate: i.i.d. from prob, or Markov from the previous index (onehot [R, K])
inline Series draw_environment(const Environment &env, const Series &previous_onehot, const Series &u) {
  const Series row = env.transition.defined() ? previous_onehot.matmul(env.transition) : previous_onehot.mul(0.0).add(env.prob);
  const Series idx = row.cumsum(-1, false).le(u.unsqueeze(-1)).to(u.scalar_type()).sum(-1, true).clamp(0.0, static_cast<double>(env.prob.size(0) - 1));
  const Series ks = pluss::series::arange(0, env.prob.size(0), 1, u.scalar_type()).to(u.platform());
  return idx.eq(ks).to(u.scalar_type());
}
inline Series apply_environment(const Environment &env, const Series &onehot, const Series &n) {
  const std::int64_t k = env.matrices.size(-1);
  const Series flat = env.matrices.reshape({env.matrices.size(0), k * k});
  const Series a = onehot.matmul(flat).reshape({onehot.size(0), k, k});
  return dynamics::matvec(a, n);
}
} // namespace detail

// Simulated stochastic growth rate: `steps` steps on `replicates` independent lanes, started at the dominant right eigenvector of the mean
// matrix, mean of ln (population size ratio) per step.
inline StochasticGrowth simulate_growth(const Environment &env, std::int64_t steps, std::int64_t replicates, std::uint64_t seed = 0) {
  dynamics::require(steps > 0 && replicates > 0, "simulate_growth: steps and replicates must be positive");
  const Config cfg = Config{}.with_dtype(env.matrices.scalar_type()).with_platform(env.matrices.platform());
  const Series mean = env.matrices.mul(env.prob.unsqueeze(-1).unsqueeze(-1)).sum(0, false);
  Series n = structured::dominant(mean).right.unsqueeze(0).expand({replicates, mean.size(-1)});
  Series onehot = pluss::series::zeros({replicates, env.prob.size(0)}, cfg).add(env.prob);
  const std::uint64_t base = pluss::tils::seed_or_next(seed);
  Series total = pluss::series::zeros({replicates}, cfg);
  Series previous = detail::draw_environment({env.matrices, env.prob, pluss::series::undefined()}, onehot, pluss::series::rand({replicates}, cfg, base));
  for (std::int64_t t = 0; t < steps; ++t) {
    const Series u = pluss::series::rand({replicates}, cfg, base + 1 + static_cast<std::uint64_t>(t));
    previous = detail::draw_environment(env, previous, u);
    n = detail::apply_environment(env, previous, n);
    const Series size = n.sum(-1, true);
    total = total.add(size.squeeze(-1).log());
    n = n.div(size);
  }
  StochasticGrowth out;
  out.per_replicate = total.div(static_cast<double>(steps));
  const auto v = out.per_replicate.contiguous().host<double>();
  double mu = 0.0, ss = 0.0;
  for (double x : v)
    mu += x;
  mu /= static_cast<double>(v.size());
  for (double x : v)
    ss += (x - mu) * (x - mu);
  out.mean = mu;
  out.standard_error = v.size() > 1 ? std::sqrt(ss / static_cast<double>(v.size() - 1) / static_cast<double>(v.size())) : 0.0;
  return out;
}

// Total population paths [R, steps + 1] from n0 [k] (all replicates share it) in the stochastic environment
inline Series simulate_paths(const Environment &env, const Series &n0, std::int64_t steps, std::int64_t replicates, std::uint64_t seed = 0) {
  const Config cfg = Config{}.with_dtype(env.matrices.scalar_type()).with_platform(env.matrices.platform());
  Series n = n0.unsqueeze(0).expand({replicates, n0.size(-1)});
  Series onehot = pluss::series::zeros({replicates, env.prob.size(0)}, cfg).add(env.prob);
  const std::uint64_t base = pluss::tils::seed_or_next(seed);
  Series previous = detail::draw_environment({env.matrices, env.prob, pluss::series::undefined()}, onehot, pluss::series::rand({replicates}, cfg, base));
  std::vector<Series> cols{n.sum(-1, true)};
  for (std::int64_t t = 0; t < steps; ++t) {
    const Series u = pluss::series::rand({replicates}, cfg, base + 1 + static_cast<std::uint64_t>(t));
    previous = detail::draw_environment(env, previous, u);
    n = detail::apply_environment(env, previous, n);
    cols.push_back(n.sum(-1, true));
  }
  std::vector<Series> rest(cols.begin() + 1, cols.end());
  return cols[0].unite(rest, 1);
}

// P(total population has fallen to or below `threshold` at some time <= t), t = 0..steps (popbio stoch.quasi.ext definition)
inline Series quasi_extinction_probability(const Series &paths, double threshold) {
  const Series hit = paths.le(threshold).to(paths.scalar_type());
  return hit.cummax(-1).mean(0, false);
}

} // namespace pluss::subsp::natsci::biology::population::demography
