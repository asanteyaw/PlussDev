#pragma once

// Upper-tail probabilities that stay accurate far in the tail. A p-value computed as 1 - cdf(x) is exactly 0 as soon as the true tail
// probability falls below about 1e-16, however extreme the statistic; these functions evaluate the tail directly (complementary error
// function, the upper regularised incomplete gamma by series / continued fraction, and the regularised incomplete beta by continued
// fraction) and keep full relative accuracy down to the smallest representable values.

#include <algorithm>
#include <cmath>
#include <limits>

namespace pluss::subsp::stats::tails {

// P(Z > z) for a standard normal Z.
inline double normal_sf(double z) { return 0.5 * std::erfc(z / std::sqrt(2.0)); }

namespace detail {

// Regularised lower incomplete gamma P(a, x) by its power series (accurate for x < a + 1).
inline double gamma_p_series(double a, double x) {
  double sum = 1.0 / a;
  double term = sum;
  for (int n = 1; n < 10000; ++n) {
    term *= x / (a + n);
    sum += term;
    if (std::abs(term) < std::abs(sum) * 1e-17) {
      break;
    }
  }
  return sum * std::exp(-x + a * std::log(x) - std::lgamma(a));
}

// Regularised upper incomplete gamma Q(a, x) by a continued fraction (Lentz), accurate for x >= a + 1.
inline double gamma_q_continued_fraction(double a, double x) {
  constexpr double tiny = 1e-300;
  double b = x + 1.0 - a;
  double c = 1.0 / tiny;
  double d = 1.0 / b;
  double h = d;
  for (int i = 1; i < 10000; ++i) {
    const double an = -static_cast<double>(i) * (static_cast<double>(i) - a);
    b += 2.0;
    d = an * d + b;
    if (std::abs(d) < tiny) d = tiny;
    c = b + an / c;
    if (std::abs(c) < tiny) c = tiny;
    d = 1.0 / d;
    const double delta = d * c;
    h *= delta;
    if (std::abs(delta - 1.0) < 1e-16) {
      break;
    }
  }
  return std::exp(-x + a * std::log(x) - std::lgamma(a)) * h;
}

// Continued fraction for the incomplete beta function (Lentz).
inline double beta_continued_fraction(double a, double b, double x) {
  constexpr double tiny = 1e-300;
  const double qab = a + b, qap = a + 1.0, qam = a - 1.0;
  double c = 1.0;
  double d = 1.0 - qab * x / qap;
  if (std::abs(d) < tiny) d = tiny;
  d = 1.0 / d;
  double h = d;
  for (int m = 1; m < 10000; ++m) {
    const double m2 = 2.0 * m;
    double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
    d = 1.0 + aa * d;
    if (std::abs(d) < tiny) d = tiny;
    c = 1.0 + aa / c;
    if (std::abs(c) < tiny) c = tiny;
    d = 1.0 / d;
    h *= d * c;
    aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
    d = 1.0 + aa * d;
    if (std::abs(d) < tiny) d = tiny;
    c = 1.0 + aa / c;
    if (std::abs(c) < tiny) c = tiny;
    d = 1.0 / d;
    const double delta = d * c;
    h *= delta;
    if (std::abs(delta - 1.0) < 1e-16) {
      break;
    }
  }
  return h;
}

} // namespace detail

// Regularised upper incomplete gamma Q(a, x) = 1 - P(a, x).
inline double gamma_q(double a, double x) {
  if (!(x > 0.0)) {
    return 1.0;
  }
  if (std::isinf(x)) {
    return 0.0;
  }
  return (x < a + 1.0) ? 1.0 - detail::gamma_p_series(a, x) : detail::gamma_q_continued_fraction(a, x);
}

// Regularised incomplete beta I_x(a, b).
inline double beta_i(double a, double b, double x) {
  if (x <= 0.0) {
    return 0.0;
  }
  if (x >= 1.0) {
    return 1.0;
  }
  const double front = std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b) + a * std::log(x) + b * std::log1p(-x));
  if (x < (a + 1.0) / (a + b + 2.0)) {
    return front * detail::beta_continued_fraction(a, b, x) / a;
  }
  return 1.0 - front * detail::beta_continued_fraction(b, a, 1.0 - x) / b;
}

// P(X > x) for chi-squared with `dof` degrees of freedom.
inline double chi2_sf(double x, double dof) { return gamma_q(0.5 * dof, 0.5 * x); }

// P(|T| > |t|) for Student's t with `dof` degrees of freedom (the two-sided p-value).
inline double student_t_two_sided(double t, double dof) {
  if (std::isnan(t)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (std::isinf(t)) {
    return 0.0;
  }
  const double x = dof / (dof + t * t);
  return std::clamp(beta_i(0.5 * dof, 0.5, x), 0.0, 1.0);
}

// P(F > f) for the F distribution with (d1, d2) degrees of freedom.
inline double f_sf(double f, double d1, double d2) {
  if (!(f > 0.0)) {
    return 1.0;
  }
  if (std::isinf(f)) {
    return 0.0;
  }
  return std::clamp(beta_i(0.5 * d2, 0.5 * d1, d2 / (d2 + d1 * f)), 0.0, 1.0);
}

} // namespace pluss::subsp::stats::tails
