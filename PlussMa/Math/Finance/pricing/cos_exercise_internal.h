#pragma once
// INTERNAL: pieces of the COS method for early exercise shared by the Black-Scholes and Heston/Bates engines
// (Fang and Oosterlee, Numer. Math. 114 (2009) 27-62; SIAM J. Financial Math. 2 (2011) 439-463).
#include <cmath>
#include <complex>
#include <numbers>
#include <vector>

namespace pluss::subsp::math::finance::internal {

// Cosine coefficients on [a, b] of the payoff alpha K (e^x - 1) over [x1, x2] (alpha = +1 call, -1 put; the interval must lie on the side
// of x = 0 where the payoff is positive): G_k of the papers.
inline std::vector<double> cos_payoff_coefficients(double a, double b, double x1, double x2, double strike, std::size_t n, double alpha) {
  std::vector<double> g(n);
  const double w = b - a;
  for (std::size_t k = 0; k < n; ++k) {
    const double om = static_cast<double>(k) * std::numbers::pi / w;
    const double c2 = std::cos(om * (x2 - a)), c1 = std::cos(om * (x1 - a)), s2 = std::sin(om * (x2 - a)), s1 = std::sin(om * (x1 - a));
    const double chi = (c2 * std::exp(x2) - c1 * std::exp(x1) + om * (s2 * std::exp(x2) - s1 * std::exp(x1))) / (1.0 + om * om);
    const double psi = k == 0 ? x2 - x1 : (s2 - s1) / om;
    g[k] = 2.0 / w * alpha * strike * (chi - psi);
  }
  return g;
}

// Coefficients over [x1, x2] of the function with cosine expansion c(x) = Re sum_n u_n exp(i n pi (x - a)/(b - a)):
//   out_k = scale * Re sum_n u_n M_{k,n}(x1, x2),   M_{k,n} = (2/(b-a)) int_{x1}^{x2} exp(i n pi (x-a)/(b-a)) cos(k pi (x-a)/(b-a)) dx.
// The integrals have the closed form (1/pi)(I(n+k) + I(n-k)), I(m) = (e^{i m t2} - e^{i m t1})/(i m) and I(0) = t2 - t1, t = pi (x-a)/(b-a).
// `scratch` holds the 4n+1 values I(m), m = -2n..2n; it is reused across calls.
inline void cos_continuation_coefficients(double a, double b, double x1, double x2, const std::vector<std::complex<double>>& u, double scale,
                                          std::vector<std::complex<double>>& scratch, std::vector<double>& out) {
  const std::size_t n = u.size();
  const double w = b - a, t1 = std::numbers::pi * (x1 - a) / w, t2 = std::numbers::pi * (x2 - a) / w;
  const long span = static_cast<long>(2 * n);
  scratch.resize(static_cast<std::size_t>(2 * span + 1));
  // I(m) for m = 0 .. span from e^{i m t} by rotation (re-synchronised with an exact value every 64 steps), then by I(-m) = conj(I(m))
  {
    const std::complex<double> r1(std::cos(t1), std::sin(t1)), r2(std::cos(t2), std::sin(t2));
    std::complex<double> e1(1.0, 0.0), e2(1.0, 0.0);
    scratch[static_cast<std::size_t>(span)] = std::complex<double>(t2 - t1, 0.0);
    for (long m = 1; m <= span; ++m) {
      if (m % 64 == 0) {
        e1 = std::complex<double>(std::cos(m * t1), std::sin(m * t1));
        e2 = std::complex<double>(std::cos(m * t2), std::sin(m * t2));
      } else {
        e1 = std::complex<double>(e1.real() * r1.real() - e1.imag() * r1.imag(), e1.real() * r1.imag() + e1.imag() * r1.real());
        e2 = std::complex<double>(e2.real() * r2.real() - e2.imag() * r2.imag(), e2.real() * r2.imag() + e2.imag() * r2.real());
      }
      const double dr = e2.real() - e1.real(), di = e2.imag() - e1.imag();
      // (e2 - e1) / (i m) = (di - i dr) / m
      const std::complex<double> value(di / static_cast<double>(m), -dr / static_cast<double>(m));
      scratch[static_cast<std::size_t>(span + m)] = value;
      scratch[static_cast<std::size_t>(span - m)] = std::conj(value);  // I(-m) = conj(I(m)) because the integrand's conjugate is the integrand at -m
    }
  }
  std::vector<double> ur(n), ui(n);
  for (std::size_t j = 0; j < n; ++j) {
    ur[j] = u[j].real();
    ui[j] = u[j].imag();
  }
  out.assign(n, 0.0);
  const double s = scale / std::numbers::pi;
  const std::complex<double>* centre = scratch.data() + span;
  for (std::size_t k = 0; k < n; ++k) {
    double acc = 0.0;
    const long kk = static_cast<long>(k);
    for (long j = 0; j < static_cast<long>(n); ++j) {
      const std::complex<double> wsum = centre[j + kk] + centre[j - kk];
      acc += ur[static_cast<std::size_t>(j)] * wsum.real() - ui[static_cast<std::size_t>(j)] * wsum.imag();
    }
    out[k] = s * acc;
  }
}

// Value and x-derivative of c(x) = scale * Re sum_n u_n exp(i n pi (x - a)/(b - a)); the exponentials come from one rotation.
inline void cos_evaluate(double a, double b, const std::vector<std::complex<double>>& u, double scale, double x, double& value, double& slope) {
  const double w = b - a, step = std::numbers::pi / w;
  const double cr = std::cos(step * (x - a)), ci = std::sin(step * (x - a));
  double er = 1.0, ei = 0.0;
  value = 0.0;
  slope = 0.0;
  for (std::size_t n = 0; n < u.size(); ++n) {
    if (n % 64 == 0 && n > 0) {
      er = std::cos(static_cast<double>(n) * step * (x - a));
      ei = std::sin(static_cast<double>(n) * step * (x - a));
    }
    const double tr = u[n].real() * er - u[n].imag() * ei, ti = u[n].real() * ei + u[n].imag() * er;
    value += tr;
    slope += -static_cast<double>(n) * step * ti;  // Re(i om term) = -om Im(term)
    const double nr = er * cr - ei * ci, ni = er * ci + ei * cr;
    er = nr;
    ei = ni;
  }
  value *= scale;
  slope *= scale;
}

// The exercise point: where the continuation value c equals the payoff alpha K (e^x - 1).  Exercise is optimal below it for a put (search
// on [a, 0]) and above it for a call (search on [0, b]); the range edge is returned when the two never cross.
template <typename Evaluate>
double cos_exercise_point(double a, double b, double strike, bool call, Evaluate&& continuation /* (x, value, slope) */) {
  const double alpha = call ? 1.0 : -1.0;
  auto gap = [&](double x, double& d) {
    double c = 0.0, s = 0.0;
    continuation(x, c, s);
    d = s - alpha * strike * std::exp(x);
    return c - alpha * strike * (std::exp(x) - 1.0);
  };
  // put: gap < 0 (exercise) on the left, >= 0 on the right of the root; call: the other way round
  double lo = call ? 0.0 : a, hi = call ? b : 0.0, d = 0.0;
  const double f_lo = gap(lo, d), f_hi = gap(hi, d);
  if (!call) {
    if (f_lo >= 0.0) return a;   // never worth exercising inside the range
    if (f_hi <= 0.0) return 0.0;
  } else {
    if (f_hi >= 0.0) return b;
    if (f_lo <= 0.0) return 0.0;
  }
  double x = 0.5 * (lo + hi);
  for (int it = 0; it < 100; ++it) {
    const double f = gap(x, d);
    const bool below = call ? f > 0.0 : f < 0.0;  // the side on which the root lies
    (below ? lo : hi) = x;
    double next = d != 0.0 ? x - f / d : 0.5 * (lo + hi);
    if (!(next > lo && next < hi)) next = 0.5 * (lo + hi);
    if (std::abs(next - x) < 1e-14 * (1.0 + std::abs(x))) {
      x = next;
      break;
    }
    x = next;
  }
  return x;
}

}  // namespace pluss::subsp::math::finance::internal
