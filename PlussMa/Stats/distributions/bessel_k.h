#pragma once

// Modified Bessel function of the second kind K_nu(x) for real order and x > 0, accurate to double precision.
// Temme's series for x < 2 and Steed's continued fraction for x >= 2 give K for an order in [-1/2, 1/2]; upward recurrence
// reaches the requested order. The scaled value exp(x) K_nu(x) never underflows, which keeps log K accurate for large x.

#include <cmath>
#include <limits>
#include <numbers>

namespace pluss::subsp::stats::distr::besselk {

namespace detail {

// 1/Gamma(1 + mu) and 1/Gamma(1 - mu) and the combinations gam1, gam2 of the Temme series, for |mu| <= 1/2.
inline void gamma_terms(double mu, double& gam1, double& gam2, double& gampl, double& gammi) {
  gampl = 1.0 / std::tgamma(1.0 + mu);
  gammi = 1.0 / std::tgamma(1.0 - mu);
  gam2 = 0.5 * (gammi + gampl);
  if (std::abs(mu) < 1e-3) {
    // (1/Gamma(1 - mu) - 1/Gamma(1 + mu)) / (2 mu) from the Taylor coefficients of 1/Gamma(1 + z): odd ones only.
    const double m2 = mu * mu;
    gam1 = -(0.5772156649015329 + m2 * (-0.0420026350340952 + m2 * (-0.0421977345555443)));
  } else {
    gam1 = (gammi - gampl) / (2.0 * mu);
  }
}

// exp(x) K_nu(x) and exp(x) K_{nu+1}(x) for nu in [-1/2, 1/2].
inline void k_pair_scaled(double mu, double x, double& rkmu, double& rk1) {
  constexpr double eps = 1e-16;
  constexpr int max_iter = 100000;
  const double mu2 = mu * mu;
  const double xi = 1.0 / x;
  const double xi2 = 2.0 * xi;
  const double pi = std::numbers::pi_v<double>;
  if (x < 2.0) {
    const double x2 = 0.5 * x;
    const double pimu = pi * mu;
    const double fact = (std::abs(pimu) < eps) ? 1.0 : pimu / std::sin(pimu);
    double d = -std::log(x2);
    double e = mu * d;
    const double fact2 = (std::abs(e) < eps) ? 1.0 : std::sinh(e) / e;
    double gam1, gam2, gampl, gammi;
    gamma_terms(mu, gam1, gam2, gampl, gammi);
    double ff = fact * (gam1 * std::cosh(e) + gam2 * fact2 * d);
    double sum = ff;
    e = std::exp(e);
    double p = 0.5 * e / gampl;
    double q = 0.5 / (e * gammi);
    double c = 1.0;
    d = x2 * x2;
    double sum1 = p;
    for (int i = 1; i <= max_iter; ++i) {
      ff = (i * ff + p + q) / (i * i - mu2);
      c *= d / i;
      p /= (i - mu);
      q /= (i + mu);
      const double del = c * ff;
      sum += del;
      sum1 += c * (p - i * ff);
      if (std::abs(del) < std::abs(sum) * eps) break;
    }
    const double scale = std::exp(x);
    rkmu = sum * scale;
    rk1 = sum1 * xi2 * scale;
  } else {
    double b = 2.0 * (1.0 + x);
    double d = 1.0 / b;
    double h = d, delh = d;
    double q1 = 0.0, q2 = 1.0;
    const double a1 = 0.25 - mu2;
    double q = a1, c = a1;
    double a = -a1;
    double s = 1.0 + q * delh;
    for (int i = 2; i <= max_iter; ++i) {
      a -= 2 * (i - 1);
      c = -a * c / i;
      const double qnew = (q1 - b * q2) / a;
      q1 = q2;
      q2 = qnew;
      q += c * qnew;
      b += 2.0;
      d = 1.0 / (b + a * d);
      delh = (b * d - 1.0) * delh;
      h += delh;
      const double dels = q * delh;
      s += dels;
      if (std::abs(dels / s) < eps) break;
    }
    h = a1 * h;
    rkmu = std::sqrt(pi / (2.0 * x)) / s;
    rk1 = rkmu * (mu + x + 0.5 - h) * xi;
  }
}

}  // namespace detail

// exp(x) K_nu(x), x > 0.
inline double k_scaled(double nu, double x) {
  if (!(x > 0.0)) {
    return std::numeric_limits<double>::infinity();
  }
  nu = std::abs(nu);  // K_{-nu} = K_nu
  const int nl = static_cast<int>(nu + 0.5);
  const double mu = nu - nl;
  double rkmu, rk1;
  detail::k_pair_scaled(mu, x, rkmu, rk1);
  const double xi2 = 2.0 / x;
  for (int i = 1; i <= nl; ++i) {
    const double next = (mu + i) * xi2 * rk1 + rkmu;
    rkmu = rk1;
    rk1 = next;
  }
  return rkmu;
}

// K_nu(x) (underflows to 0 for very large x).
inline double k(double nu, double x) {
  return (x > 0.0) ? k_scaled(nu, x) * std::exp(-x) : std::numeric_limits<double>::infinity();
}

// log K_nu(x), accurate for large x.
inline double log_k(double nu, double x) {
  return (x > 0.0) ? std::log(k_scaled(nu, x)) - x : std::numeric_limits<double>::infinity();
}

}  // namespace pluss::subsp::stats::distr::besselk
