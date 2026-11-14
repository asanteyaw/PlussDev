// Bermudan and American options under Heston and Bates by the COS method of Fang and Oosterlee, "A Fourier-based valuation method for
// Bermudan and barrier options under Heston's model", SIAM J. Financial Math. 2 (2011) 439-463.
//
// The value is carried between exercise dates as cosine coefficients V_{n,j} in the log-asset dimension for each node j of a Gauss-Legendre
// grid in the log-variance dimension.  One step multiplies the coefficients by the kernel phi~(omega, sigma_to, sigma_from) = (density of the
// log-variance) x (characteristic function of the log-asset given both log-variances), in which the two Bessel functions cancel to leave a
// single I_q of complex argument.
#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "bessel_complex_internal.h"
#include "cos_exercise_internal.h"
#include "parallel_internal.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/stats/distr_fwd.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <map>
#include <numbers>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::pricing {
using namespace detail;
namespace {

using internal::bessel_i_parts_logz;
using internal::cos_continuation_coefficients;
using internal::cos_evaluate;
using internal::cos_exercise_point;
using internal::cos_payoff_coefficients;
using Complex = std::complex<double>;
using Vec = std::vector<double>;

struct Model {
  double kappa, theta, eta, rho, v0, rate, carry;          // carry = dividend yield
  double jump_intensity = 0.0, jump_mean = 0.0, jump_vol = 0.0;  // Bates; intensity 0 is Heston

  double jump_compensator() const { return std::exp(jump_mean + 0.5 * jump_vol * jump_vol) - 1.0; }
  double drift() const { return rate - carry - jump_intensity * jump_compensator(); }

  Complex jump_factor(double omega, double dt) const {
    if (jump_intensity == 0.0) return Complex(1.0, 0.0);
    return std::exp(jump_intensity * dt * (std::exp(Complex(-0.5 * jump_vol * jump_vol * omega * omega, omega * jump_mean)) - 1.0));
  }

  // characteristic function of the log-return over t (Heston with the jump factor), for the cumulants that size the log-asset range
  Complex log_return_cf(double omega, double t) const {
    const Complex i(0.0, 1.0), b = kappa - rho * eta * i * omega;
    const Complex d = std::sqrt(b * b + eta * eta * (i * omega + omega * omega));
    const Complex g = (b - d) / (b + d), e = std::exp(-d * t);
    const Complex c = kappa * theta / (eta * eta) * ((b - d) * t - 2.0 * std::log((1.0 - g * e) / (1.0 - g)));
    const Complex dd = v0 / (eta * eta) * (b - d) * (1.0 - e) / (1.0 - g * e);
    return std::exp(i * omega * drift() * t + c + dd) * jump_factor(omega, t);
  }
};

// The kernel phi~(omega, sigma_to, sigma_from) for a step of length dt.  Everything that does not depend on the pair of nodes is computed once per
// (step, frequency) in Frequency, and everything that depends on one node only once per node in Nodes; a pair then costs one Bessel evaluation and
// one complex exponential.
//   phi~ = jump * exp(e1 + e2 + e3 + e4) * I_q(z),   z = sqrt(nu_s nu_t) * 4 gamma e^{-gamma dt/2} / (eta^2 (1 - e^{-gamma dt}))
// with e1 the log-variance transition density without its Bessel factor, e2 the drift and correlation terms of the log-asset, e3 and e4 the
// variance-integrated characteristic function without its Bessel factor.
struct Step {
  double dt, lambda, eta2, q, zeta, el, constant1;  // constant1: the node-free part of e1
  internal::BesselTables bessel, bessel1;           // tables of the orders q and q + 1 (the second gives the derivative in the argument)
};

Step make_step(const Model& m, double dt) {
  const double order = 2.0 * m.kappa * m.theta / (m.eta * m.eta) - 1.0;
  Step s{.bessel = internal::BesselTables(order), .bessel1 = internal::BesselTables(order + 1.0)};
  s.dt = dt;
  s.lambda = m.kappa;
  s.eta2 = m.eta * m.eta;
  s.q = 2.0 * s.lambda * m.theta / s.eta2 - 1.0;
  s.zeta = 2.0 * s.lambda / ((1.0 - std::exp(-s.lambda * dt)) * s.eta2);
  s.el = std::exp(-s.lambda * dt);
  s.constant1 = std::log(s.zeta) + 0.5 * s.q * s.lambda * dt;
  return s;
}

struct Nodes {  // per node, for a given step
  Vec sigma, nu, half, to_term, from_term;  // sigma = ln nu, half = sqrt(nu), to/from_term: the node parts of e1 as target / as source
};

Nodes make_nodes(const Step& st, const Vec& sigma) {
  Nodes n;
  n.sigma = sigma;
  n.nu.resize(sigma.size());
  n.half.resize(sigma.size());
  n.to_term.resize(sigma.size());
  n.from_term.resize(sigma.size());
  for (std::size_t j = 0; j < sigma.size(); ++j) {
    n.nu[j] = std::exp(sigma[j]);
    n.half[j] = std::exp(0.5 * sigma[j]);
    n.to_term[j] = sigma[j] * (1.0 + 0.5 * st.q) - st.zeta * n.nu[j];
    n.from_term[j] = -st.zeta * st.el * n.nu[j] - 0.5 * st.q * sigma[j];
  }
  return n;
}

struct Frequency {
  Complex constant;  // e2's constant part + e3 + e1's node-free part
  double correlation;  // omega rho / eta: multiplies i (nu_t - nu_s)
  Complex c4;          // multiplies (nu_s + nu_t) / eta^2
  Complex z_scale;     // z = sqrt(nu_s nu_t) * z_scale
  double log_abs_z_scale, arg_z_scale;
  Complex jump;
  bool has_jump;
};

Frequency make_frequency(const Model& m, const Step& st, double omega) {
  const Complex i(0.0, 1.0);
  const Complex upsilon(omega * (st.lambda * m.rho / m.eta - 0.5), 0.5 * omega * omega * (1.0 - m.rho * m.rho));
  const Complex gamma = std::sqrt(st.lambda * st.lambda - 2.0 * i * st.eta2 * upsilon);
  const Complex eg = std::exp(-gamma * st.dt);
  Frequency f;
  f.constant = st.constant1 + i * omega * (m.drift() * st.dt - m.rho / m.eta * st.lambda * m.theta * st.dt) +
               (std::log(gamma) - 0.5 * (gamma - st.lambda) * st.dt + std::log(1.0 - st.el) - std::log(st.lambda) - std::log(1.0 - eg));
  f.correlation = omega * m.rho / m.eta;
  f.c4 = st.lambda * (1.0 + st.el) / (1.0 - st.el) - gamma * (1.0 + eg) / (1.0 - eg);
  f.z_scale = 4.0 * gamma * std::exp(-0.5 * gamma * st.dt) / (st.eta2 * (1.0 - eg));
  f.log_abs_z_scale = std::log(std::abs(f.z_scale));
  f.arg_z_scale = std::arg(f.z_scale);
  f.jump = m.jump_factor(omega, st.dt);
  f.has_jump = m.jump_intensity != 0.0;
  return f;
}

inline Complex kernel_pair(const Step& st, const Nodes& nd, const Frequency& f, std::size_t to, std::size_t from) {
  const double hh = nd.half[to] * nd.half[from];  // sqrt(nu_s nu_t); its logarithm is the mean of the two log-variances
  const Complex z(hh * f.z_scale.real(), hh * f.z_scale.imag());
  const Complex log_z(0.5 * (nd.sigma[to] + nd.sigma[from]) + f.log_abs_z_scale, f.arg_z_scale);
  Complex log_scale;
  const Complex value = bessel_i_parts_logz(st.bessel, z, log_z, log_scale);
  const double sum = (nd.nu[from] + nd.nu[to]) / st.eta2;
  const double re = f.constant.real() + nd.to_term[to] + nd.from_term[from] + sum * f.c4.real() + log_scale.real();
  const double im = f.constant.imag() + f.correlation * (nd.nu[to] - nd.nu[from]) + sum * f.c4.imag() + log_scale.imag();
  const double magnitude = std::exp(re);
  const double sin_im = std::sin(im), cos_im = std::cos(im);
  const double er = magnitude * cos_im, ei = magnitude * sin_im;
  double rr = er * value.real() - ei * value.imag(), ri = er * value.imag() + ei * value.real();
  if (f.has_jump) {
    const double jr = rr * f.jump.real() - ri * f.jump.imag(), ji = rr * f.jump.imag() + ri * f.jump.real();
    rr = jr;
    ri = ji;
  }
  return Complex(rr, ri);
}

// One value of the kernel (the first step from the initial variance, and the resolution check).
Complex kernel(const Model& m, double omega, double dt, double sigma_to, double sigma_from) {
  const Step st = make_step(m, dt);
  const Nodes nd = make_nodes(st, Vec{sigma_to, sigma_from});
  return kernel_pair(st, nd, make_frequency(m, st, omega), 0, 1);
}

// The kernel and its first and second derivatives in the log-variance of the source node: d/d sigma_from log phi~ = E0' + (z/2) I'_q(z)/I_q(z), with
// I'_q = I_{q+1} + (q/z) I_q and I''_q from Bessel's equation. Exact, no differencing.
struct KernelDerivatives {
  Complex k0, k1, k2;
};

KernelDerivatives kernel_sigma_derivatives(const Step& st, const Nodes& nd, const Frequency& f, std::size_t to, std::size_t from) {
  const Complex i(0.0, 1.0);
  const Complex k0 = kernel_pair(st, nd, f, to, from);
  const double hh = nd.half[to] * nd.half[from];
  const Complex z(hh * f.z_scale.real(), hh * f.z_scale.imag());
  const Complex log_z(0.5 * (nd.sigma[to] + nd.sigma[from]) + f.log_abs_z_scale, f.arg_z_scale);
  Complex ls0, ls1;
  const Complex i0 = bessel_i_parts_logz(st.bessel, z, log_z, ls0), i1 = bessel_i_parts_logz(st.bessel1, z, log_z, ls1);
  const Complex ratio = i1 / i0 * std::exp(ls1 - ls0);                       // I_{q+1}(z) / I_q(z)
  const Complex r = ratio + st.q / z;                                        // I'_q / I_q
  const double nu = nd.nu[from];
  const Complex d_e = Complex(-st.zeta * st.el * nu - 0.5 * st.q, 0.0) + nu / st.eta2 * f.c4 - i * (f.correlation * nu);
  const Complex dd_e = Complex(-st.zeta * st.el * nu, 0.0) + nu / st.eta2 * f.c4 - i * (f.correlation * nu);
  const Complex r_z = 1.0 + st.q * st.q / (z * z) - r / z - r * r;
  const Complex d_f = d_e + 0.5 * z * r;
  const Complex dd_f = dd_e + 0.25 * z * r + 0.25 * z * z * r_z;
  return {k0, k0 * d_f, k0 * (dd_f + d_f * d_f)};
}

// Value, slope and curvature of c(x) = scale * Re sum_n u_n exp(i n pi (x - a)/(b - a)).
inline void cos_evaluate2(double a, double b, const std::vector<Complex>& u, double scale, double x, double& value, double& slope, double& curvature) {
  const double step = std::numbers::pi / (b - a);
  value = slope = curvature = 0.0;
  for (std::size_t n = 0; n < u.size(); ++n) {
    const double om = static_cast<double>(n) * step, ang = om * (x - a);
    const double er = std::cos(ang), ei = std::sin(ang);
    const double tr = u[n].real() * er - u[n].imag() * ei, ti = u[n].real() * ei + u[n].imag() * er;
    value += tr;
    slope += -om * ti;
    curvature += -om * om * tr;
  }
  value *= scale;
  slope *= scale;
  curvature *= scale;
}

struct Gauss {
  Vec x, w;
};

Gauss gauss_legendre(std::size_t n) {
  Gauss g;
  g.x.assign(n, 0.0);
  g.w.assign(n, 0.0);
  const double nd = static_cast<double>(n);
  for (std::size_t i = 0; i < (n + 1) / 2; ++i) {
    double z = std::cos(std::numbers::pi * (static_cast<double>(i) + 0.75) / (nd + 0.5)), dp = 1.0;
    for (int it = 0; it < 100; ++it) {
      double p0 = 1.0, p1 = z;
      for (std::size_t k = 2; k <= n; ++k) {
        const double p2 = ((2.0 * static_cast<double>(k) - 1.0) * z * p1 - (static_cast<double>(k) - 1.0) * p0) / static_cast<double>(k);
        p0 = p1;
        p1 = p2;
      }
      dp = nd * (z * p1 - p0) / (z * z - 1.0);
      const double dz = p1 / dp;
      z -= dz;
      if (std::abs(dz) < 1e-16) break;
    }
    g.x[i] = -z;
    g.x[n - 1 - i] = z;
    g.w[i] = g.w[n - 1 - i] = 2.0 / ((1.0 - z * z) * dp * dp);
  }
  return g;
}

// P(X <= x) for X ~ noncentral chi-square with df degrees of freedom and noncentrality nc, from the library's distributions (which take the
// noncentrality first, then the degrees of freedom)
double ncx2_cdf(double x, double df, double nc) {
  return pluss::subsp::stats::distr::cdf(pluss::subsp::stats::distr::dst::ChiSquareNoncentral, pluss::make_series({x}), std::initializer_list<double>{nc, df})
      .items<double>()[0];
}

// quantile by bisection on the library's cdf
double ncx2_quantile(double p, double df, double nc) {
  double lo = 0.0, hi = df + nc + 10.0;
  while (ncx2_cdf(hi, df, nc) < p) hi *= 2.0;
  for (int it = 0; it < 80; ++it) {
    const double mid = 0.5 * (lo + hi);
    (ncx2_cdf(mid, df, nc) < p ? lo : hi) = mid;
  }
  return 0.5 * (lo + hi);
}

// The option value, its first and second derivatives in x = ln(S / K) and its derivative in the initial variance, from the final expansion.
struct Triple {
  double v = 0.0, vx = 0.0, vxx = 0.0, vega = 0.0, rho = 0.0, theta = 0.0;   // theta = -dV/d(time to the first date): the passage of calendar time
};

class Bermudan {
 public:
  Bermudan(const Model& m, bool call, double spot, double strike, const Vec& times, const CosOptions& o)
      : m_(m), call_(call), strike_(strike), times_(times), n_(static_cast<std::size_t>(o.exercise_terms)), j_(static_cast<std::size_t>(o.variance_nodes)) {
    const double horizon = times.back(), x0 = std::log(spot / strike);
    x0_ = x0;
    const double h = 1e-3;
    const Complex lp = std::log(m.log_return_cf(h, horizon)), lm = std::log(m.log_return_cf(-h, horizon)), l0 = std::log(m.log_return_cf(1e-12, horizon));
    const double c1 = ((lp - lm) / (2.0 * h * Complex(0.0, 1.0))).real(), c2 = std::max(-((lp + lm - 2.0 * l0) / (h * h)).real(), 1e-8);
    a_ = x0 + c1 - o.exercise_truncation * std::sqrt(c2);
    b_ = x0 + c1 + o.exercise_truncation * std::sqrt(c2);
    a_ = std::min(a_, std::min(x0, 0.0) - 0.5);
    b_ = std::max(b_, std::max(x0, 0.0) + 0.5);
    build_variance_grid_(o);
  }

  Triple price() {
    const std::size_t dates = times_.size();
    const Vec payoff = call_ ? cos_payoff_coefficients(a_, b_, 0.0, b_, strike_, n_, 1.0) : cos_payoff_coefficients(a_, b_, a_, 0.0, strike_, n_, -1.0);
    std::vector<Vec> v(n_, Vec(j_));  // v[n][j]
    for (std::size_t n = 0; n < n_; ++n) std::fill(v[n].begin(), v[n].end(), payoff[n]);
    std::vector<Vec> dv(n_, Vec(j_, 0.0));  // derivative of v in the rate; the exercise points are held fixed (the value is stationary in them)
    for (std::size_t mdx = dates - 1; mdx >= 1; --mdx) {  // dates M-1 .. 1; step from date mdx to mdx+1 has length times[mdx+1]-times[mdx]
      const double dt = times_[mdx] - times_[mdx - 1];
      const std::vector<Complex>& phi = kernel_tensor_(dt);
      const double disc = std::exp(-m_.rate * dt);
      std::vector<Vec> next(n_, Vec(j_)), dnext(n_, Vec(j_));
      std::vector<std::vector<Complex>> beta(j_, std::vector<Complex>(n_)), dbeta(j_, std::vector<Complex>(n_));
      const double work = static_cast<double>(n_ * j_ * j_) * 8.0;
      internal::parallel_for(j_, work, [&](std::size_t begin, std::size_t end) {  // beta_n(p) = sum_j w_j V_{n,j} phi~_n(j, p)
        for (std::size_t p = begin; p < end; ++p)
          for (std::size_t n = 0; n < n_; ++n) {
            Complex acc(0.0, 0.0), dacc(0.0, 0.0);
            const Complex* row = &phi[(n * j_ + p) * j_];
            for (std::size_t jj = 0; jj < j_; ++jj) {
              acc += w_[jj] * v[n][jj] * row[jj];
              dacc += w_[jj] * dv[n][jj] * row[jj];
            }
            // the rate enters the kernel through the drift only, a factor exp(i omega drift dt) common to every pair of nodes
            dacc += Complex(0.0, static_cast<double>(n) * std::numbers::pi / (b_ - a_) * dt) * acc;
            beta[p][n] = n == 0 ? 0.5 * acc : acc;
            dbeta[p][n] = n == 0 ? 0.5 * dacc : dacc;
          }
      });
      internal::parallel_for(j_, static_cast<double>(n_ * n_ * j_) * 8.0, [&](std::size_t begin, std::size_t end) {  // exercise point and coefficients per node
        std::vector<Complex> scratch;
        Vec cont, dcont;
        for (std::size_t p = begin; p < end; ++p) {
          const auto& u = beta[p];
          const double xs = cos_exercise_point(a_, b_, strike_, call_, [&](double x, double& val, double& slope) { cos_evaluate(a_, b_, u, disc, x, val, slope); });
          if (call_) {
            cos_continuation_coefficients(a_, b_, a_, xs, u, disc, scratch, cont);
            const Vec g = cos_payoff_coefficients(a_, b_, xs, b_, strike_, n_, 1.0);
            cos_continuation_coefficients(a_, b_, a_, xs, dbeta[p], disc, scratch, dcont);
            for (std::size_t n = 0; n < n_; ++n) { next[n][p] = cont[n] + g[n]; dnext[n][p] = dcont[n] - dt * cont[n]; }
          } else {
            cos_continuation_coefficients(a_, b_, xs, b_, u, disc, scratch, cont);
            const Vec g = cos_payoff_coefficients(a_, b_, a_, xs, strike_, n_, -1.0);
            cos_continuation_coefficients(a_, b_, xs, b_, dbeta[p], disc, scratch, dcont);
            for (std::size_t n = 0; n < n_; ++n) { next[n][p] = cont[n] + g[n]; dnext[n][p] = dcont[n] - dt * cont[n]; }
          }
        }
      });
      v = std::move(next);
      dv = std::move(dnext);
    }
    // from today (variance v0, which need not be a grid node) to the first date
    const double dt = times_[0];
    const Step st = make_step(m_, dt);
    // the last node of the source grid is the initial variance, which need not be on the grid: the kernel and its first two derivatives in the
    // log-variance are exact there
    Vec all = sigma_;
    all.push_back(std::log(m_.v0));
    const Nodes nd = make_nodes(st, all);
    std::vector<Complex> u(n_), u1(n_), u2(n_), ur(n_);
    for (std::size_t n = 0; n < n_; ++n) {
      const double omega = static_cast<double>(n) * std::numbers::pi / (b_ - a_);
      const Frequency f = make_frequency(m_, st, omega);
      Complex a0(0.0, 0.0), a1(0.0, 0.0), a2(0.0, 0.0), ar(0.0, 0.0);
      for (std::size_t jj = 0; jj < j_; ++jj) {
        const KernelDerivatives k = kernel_sigma_derivatives(st, nd, f, jj, j_);
        a0 += w_[jj] * v[n][jj] * k.k0;
        a1 += w_[jj] * v[n][jj] * k.k1;
        a2 += w_[jj] * v[n][jj] * k.k2;
        ar += w_[jj] * dv[n][jj] * k.k0;
      }
      ar += Complex(0.0, omega * dt) * a0;
      const double half = n == 0 ? 0.5 : 1.0;
      u[n] = half * a0;
      u1[n] = half * a1;
      u2[n] = half * a2;
      ur[n] = half * ar;
    }
    const double disc0 = std::exp(-m_.rate * dt), v0 = m_.v0;
    double value = 0.0, vx = 0.0, vxx = 0.0, ignore = 0.0, ignore2 = 0.0;
    cos_evaluate2(a_, b_, u, disc0, x0_, value, vx, vxx);
    double v_s = 0.0, v_xs = 0.0, v_ss = 0.0, v_r = 0.0;
    cos_evaluate2(a_, b_, u1, disc0, x0_, v_s, v_xs, ignore);
    cos_evaluate2(a_, b_, u2, disc0, x0_, v_ss, ignore, ignore2);
    cos_evaluate2(a_, b_, ur, disc0, x0_, v_r, ignore, ignore2);
    v_r -= dt * value;                                           // the discount factor
    const double v_v = v_s / v0, v_xv = v_xs / v0, v_vv = (v_ss - v_s) / (v0 * v0);   // log-variance to variance
    // calendar time: dV/d(time to the first date) from the pricing equation (the value is a conditional expectation over that step)
    const double jump_k = m_.jump_intensity != 0.0 ? m_.jump_compensator() : 0.0;
    double jump_term = 0.0;
    if (m_.jump_intensity != 0.0) {
      const Gauss g = gauss_legendre(96);
      const double lo = m_.jump_mean - 9.0 * m_.jump_vol, hi = m_.jump_mean + 9.0 * m_.jump_vol;
      double expect = 0.0;
      for (std::size_t k = 0; k < g.x.size(); ++k) {
        const double jx = 0.5 * (hi - lo) * g.x[k] + 0.5 * (hi + lo);
        const double dens = std::exp(-0.5 * (jx - m_.jump_mean) * (jx - m_.jump_mean) / (m_.jump_vol * m_.jump_vol)) / (m_.jump_vol * std::sqrt(2.0 * std::numbers::pi));
        const double x = x0_ + jx;
        double val = 0.0;
        if (x >= a_ && x <= b_) {
          cos_evaluate2(a_, b_, u, disc0, x, val, ignore, ignore2);
        } else {
          const double payoff = call_ ? strike_ * (std::exp(x) - 1.0) : strike_ * (1.0 - std::exp(x));
          val = (call_ ? x > b_ : x < a_) ? payoff : 0.0;
        }
        expect += 0.5 * (hi - lo) * g.w[k] * dens * val;
      }
      jump_term = m_.jump_intensity * (expect - value);
    }
    const double drift_x = m_.rate - m_.carry - m_.jump_intensity * jump_k - 0.5 * v0;
    const double v_tau = drift_x * vx + 0.5 * v0 * vxx + m_.rho * m_.eta * v0 * v_xv + 0.5 * m_.eta * m_.eta * v0 * v_vv + m_.kappa * (m_.theta - v0) * v_v -
                         m_.rate * value + jump_term;
    return Triple{value, vx, vxx, v_v, v_r, -v_tau};
  }

 private:
  Model m_;
  bool call_;
  double strike_, a_, b_, x0_;
  Vec times_;
  std::size_t n_, j_;
  Vec sigma_, w_;  // log-variance nodes and quadrature weights
  std::size_t bulk_first_ = 0, bulk_last_ = 0;  // the nodes [first, last) cover the bulk of the variance distribution
  std::map<long long, std::vector<Complex>> tensors_;

  // The log-variance grid: a composite Gauss-Legendre rule.  The range [a_nu, b_nu] holds all but o.variance_tolerance of the probability at
  // the exercise dates, but for parameters that violate the Feller condition its left tail is very heavy (the range can reach -30), while the
  // part that matters is the bulk between the 1e-5 quantiles.  So the bulk gets most of the nodes and each tail a few.
  void build_variance_grid_(const CosOptions& o) {
    const double df = 4.0 * m_.kappa * m_.theta / (m_.eta * m_.eta);
    double lo = 1e300, hi = -1e300, lo_bulk = 1e300, hi_bulk = -1e300;
    std::vector<double> probe{times_.front(), times_.back(), times_[times_.size() / 2], times_[times_.size() / 4], times_[(3 * times_.size()) / 4]};
    for (const double t : probe) {
      const double zeta = 2.0 * m_.kappa / ((1.0 - std::exp(-m_.kappa * t)) * m_.eta * m_.eta), nc = 2.0 * zeta * m_.v0 * std::exp(-m_.kappa * t);
      auto log_quantile = [&](double p) { return std::log(ncx2_quantile(p, df, nc) / (2.0 * zeta)); };
      lo = std::min(lo, log_quantile(o.variance_tolerance));
      hi = std::max(hi, log_quantile(1.0 - o.variance_tolerance));
      lo_bulk = std::min(lo_bulk, log_quantile(1e-5));
      hi_bulk = std::max(hi_bulk, log_quantile(1.0 - 1e-5));
    }
    const double v0 = std::log(m_.v0);
    lo = std::min(lo, v0 - 0.5);
    hi = std::max(hi, v0 + 0.5);
    lo_bulk = std::clamp(std::min(lo_bulk, v0 - 0.25), lo, hi);
    hi_bulk = std::clamp(std::max(hi_bulk, v0 + 0.25), lo, hi);
    const std::size_t left = std::max<std::size_t>(lo_bulk > lo + 1e-9 ? 4 : 0, static_cast<std::size_t>(0.2 * static_cast<double>(j_)));
    const std::size_t right = std::max<std::size_t>(hi > hi_bulk + 1e-9 ? 4 : 0, static_cast<std::size_t>(0.1 * static_cast<double>(j_)));
    const std::size_t left_n = lo_bulk > lo + 1e-9 ? left : 0, right_n = hi > hi_bulk + 1e-9 ? right : 0, bulk_n = j_ - left_n - right_n;
    sigma_.clear();
    w_.clear();
    bulk_first_ = left_n;
    bulk_last_ = left_n + bulk_n;
    auto panel = [&](std::size_t n, double from, double to) {
      if (n == 0) return;
      const Gauss g = gauss_legendre(n);
      for (std::size_t j = 0; j < n; ++j) {
        sigma_.push_back(0.5 * (to - from) * g.x[j] + 0.5 * (to + from));
        w_.push_back(0.5 * (to - from) * g.w[j]);
      }
    };
    panel(left_n, lo, lo_bulk);
    panel(bulk_n, lo_bulk, hi_bulk);
    panel(right_n, hi_bulk, hi);
  }

  // The kernel at omega = 0 is the transition density of the log-variance: its quadrature over the grid must be 1 for every source node
  // whose transition stays in range.  Otherwise the Gauss-Legendre grid does not resolve the (narrow) density at this step length, and the
  // recursion would return a wrong price without any other sign.
  void check_resolution_(double dt) const {
    const Step st = make_step(m_, dt);
    const Nodes nd = make_nodes(st, sigma_);
    const Frequency f = make_frequency(m_, st, 0.0);
    const std::size_t trim = (bulk_last_ - bulk_first_) / 10;  // sources at the very edge of the bulk lose a little probability beyond the grid by design
    for (std::size_t from = bulk_first_ + trim; from + trim < bulk_last_; ++from) {
      double mass = 0.0;
      for (std::size_t to = 0; to < j_; ++to) mass += w_[to] * kernel_pair(st, nd, f, to, from).real();
      PLUSS_CHECK(std::abs(mass - 1.0) < 2e-4,
                  "subsp::math::finance::pricing::heston_price_cos_exercise: the variance quadrature does not resolve the transition density for a step of " + std::to_string(dt) +
                      " years (it integrates to " + std::to_string(mass) + " at log-variance " + std::to_string(sigma_[from]) +
                      "); raise variance_nodes or use fewer exercise dates / a smaller extrapolation depth");
    }
  }

  // phi~ at every cosine frequency and pair of nodes, for a step of length dt: index (n * J + from) * J + to
  const std::vector<Complex>& kernel_tensor_(double dt) {
    const long long key = std::llround(dt * 1e12);
    auto it = tensors_.find(key);
    if (it != tensors_.end()) return it->second;
    std::vector<Complex> t(n_ * j_ * j_);
    check_resolution_(dt);
    const Step st = make_step(m_, dt);
    const Nodes nd = make_nodes(st, sigma_);
    internal::parallel_for(n_, static_cast<double>(n_ * j_ * j_) * 150.0, [&](std::size_t begin, std::size_t end) {
      for (std::size_t n = begin; n < end; ++n) {
        const Frequency f = make_frequency(m_, st, static_cast<double>(n) * std::numbers::pi / (b_ - a_));
        for (std::size_t from = 0; from < j_; ++from) {
          Complex* row = &t[(n * j_ + from) * j_];
          for (std::size_t to = 0; to < j_; ++to) row[to] = kernel_pair(st, nd, f, to, from);
        }
      }
    });
    return tensors_.emplace(key, std::move(t)).first->second;
  }
};

Triple bermudan_value(const Model& m, bool call, double spot, double strike, const Vec& times, const CosOptions& o) {
  return Bermudan(m, call, spot, strike, times, o).price();
}

Triple triple_with(const Model& m, OptionType type, ExerciseStyle style, double spot, double strike, double maturity, std::span<const double> dates, const CosOptions& o,
                  const char* fn) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put, std::string("subsp::math::finance::pricing::") + fn + ": option type must be Call or Put");
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0 && std::isfinite(strike) && strike > 0.0,
              std::string("subsp::math::finance::pricing::") + fn + ": spot and strike must be finite and > 0");
  PLUSS_CHECK(m.kappa > 0.0 && m.theta > 0.0 && m.eta > 0.0 && m.v0 > 0.0 && m.rho > -1.0 && m.rho < 1.0,
              std::string("subsp::math::finance::pricing::") + fn + ": Heston parameters out of range (the initial variance must be > 0 for the log-variance grid)");
  PLUSS_CHECK(o.exercise_terms >= 8 && o.variance_nodes >= 8 && o.extrapolation >= -1 && o.extrapolation <= 6,
              std::string("subsp::math::finance::pricing::") + fn + ": exercise_terms >= 8, variance_nodes >= 8 and extrapolation in [-1, 6] are required");
  if (!(maturity > 0.0)) return Triple{type == OptionType::Call ? std::max(spot - strike, 0.0) : std::max(strike - spot, 0.0)};
  const bool call = type == OptionType::Call;
  if (style == ExerciseStyle::European) return bermudan_value(m, call, spot, strike, Vec{maturity}, o);
  if (style == ExerciseStyle::Bermudan) {
    Vec times;
    for (const double t : dates) {
      PLUSS_CHECK(std::isfinite(t) && t > 0.0, std::string("subsp::math::finance::pricing::") + fn + ": exercise dates must be finite and > 0");
      if (t < maturity) times.push_back(t);
    }
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end(), [](double x, double y) { return std::abs(x - y) < 1e-12; }), times.end());
    times.push_back(maturity);
    return bermudan_value(m, call, spot, strike, times, o);
  }
  auto equally_spaced = [&](std::int64_t count) {
    Vec times(static_cast<std::size_t>(count));
    for (std::int64_t i = 0; i < count; ++i) times[static_cast<std::size_t>(i)] = maturity * static_cast<double>(i + 1) / static_cast<double>(count);
    return bermudan_value(m, call, spot, strike, times, o);
  };
  const std::int64_t base = std::int64_t{1} << (o.extrapolation < 0 ? 2 : o.extrapolation);
  const Triple a = equally_spaced(8 * base), b = equally_spaced(4 * base), c = equally_spaced(2 * base), d = equally_spaced(base);
  const auto mix = [&](double x8, double x4, double x2, double x1) { return (64.0 * x8 - 56.0 * x4 + 14.0 * x2 - x1) / 21.0; };
  return Triple{mix(a.v, b.v, c.v, d.v), mix(a.vx, b.vx, c.vx, d.vx), mix(a.vxx, b.vxx, c.vxx, d.vxx), mix(a.vega, b.vega, c.vega, d.vega), mix(a.rho, b.rho, c.rho, d.rho), mix(a.theta, b.theta, c.theta, d.theta)};
}

double price_with(const Model& m, OptionType type, ExerciseStyle style, double spot, double strike, double maturity, std::span<const double> dates, const CosOptions& o,
                  const char* fn) {
  return triple_with(m, type, style, spot, strike, maturity, dates, o, fn).v;
}

ScalarGreeks greeks_with(const Model& m, OptionType type, ExerciseStyle style, double spot, double strike, double maturity, std::span<const double> dates, const CosOptions& o,
                         const char* fn) {
  ScalarGreeks g;
  const Triple base = triple_with(m, type, style, spot, strike, maturity, dates, o, fn);
  g.price = base.v;
  if (!(maturity > 0.0)) {
    g.delta = type == OptionType::Call ? (spot > strike ? 1.0 : 0.0) : (spot < strike ? -1.0 : 0.0);
    return g;
  }
  g.delta = base.vx / spot;
  g.gamma = (base.vxx - base.vx) / (spot * spot);
  g.vega = base.vega;
  g.rho = base.rho;
  g.theta = base.theta;
  return g;
}

}  // namespace

double detail::heston_price_cos_exercise(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                 const volatility::HestonParams& p, std::span<const double> dates, const CosOptions& o) {
  const Model m{p.kappa, p.theta, p.sigma, p.rho, p.v0, rate, dividend_yield};
  return price_with(m, type, style, spot, strike, maturity, dates, o, "heston_price_cos_exercise");
}

double detail::bates_price_cos_exercise(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                const volatility::BatesParams& p, std::span<const double> dates, const CosOptions& o) {
  PLUSS_CHECK(p.jump_intensity >= 0.0 && p.jump_vol >= 0.0 && std::isfinite(p.jump_mean),
              "subsp::math::finance::pricing::bates_price_cos_exercise: jump parameters out of range");
  const Model m{p.kappa, p.theta, p.sigma, p.rho, p.v0, rate, dividend_yield, p.jump_intensity, p.jump_mean, p.jump_vol};
  return price_with(m, type, style, spot, strike, maturity, dates, o, "bates_price_cos_exercise");
}

ScalarGreeks detail::heston_cos_exercise_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                                const volatility::HestonParams& p, std::span<const double> dates, const CosOptions& o) {
  const Model m{p.kappa, p.theta, p.sigma, p.rho, p.v0, rate, dividend_yield};
  return greeks_with(m, type, style, spot, strike, maturity, dates, o, "heston_price_cos_exercise");
}

ScalarGreeks detail::bates_cos_exercise_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                               const volatility::BatesParams& p, std::span<const double> dates, const CosOptions& o) {
  const Model m{p.kappa, p.theta, p.sigma, p.rho, p.v0, rate, dividend_yield, p.jump_intensity, p.jump_mean, p.jump_vol};
  return greeks_with(m, type, style, spot, strike, maturity, dates, o, "bates_price_cos_exercise");
}

}  // namespace pluss::subsp::math::finance::pricing
