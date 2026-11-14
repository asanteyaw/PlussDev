#pragma once
// INTERNAL: modified Bessel function of the first kind for complex argument and real order, scaled by exp(-z).
//
//   bessel_i_scaled(nu, z) = exp(-z) I_nu(z),   Re z >= 0,  nu > -1.
//
// Power series for |z| < 18 and Hankel's asymptotic expansion beyond.  In the right half-plane near the real axis (|arg z| <= pi/4,
// which is where the Heston characteristic function lives) the neglected exp(-2z) term of the expansion is below 1e-11 for |z| >= 18,
// and the cancellation in the series costs at most a factor exp(|z| - Re z) <= exp(0.3 |z|) in the scaled value.  Accuracy ~1e-12.
#include <cmath>
#include <complex>
#include <vector>

namespace pluss::subsp::math::finance::internal {

// I_nu(z) = exp(log_scale) * value, with the exponent kept apart so that callers can fold it into a larger exponential without overflow.
// `inverse_gamma` is 1 / Gamma(nu + 1), which depends on the order only: callers evaluating many arguments at one order pass it in.
// The loops use explicit real arithmetic and squared norms: they run millions of times in the Heston early-exercise kernel, where the
// library's complex multiplication (with its NaN recovery path), complex division and hypot dominated the cost.
inline std::complex<double> bessel_i_parts(double nu, std::complex<double> z, std::complex<double>& log_scale, double inverse_gamma) {
  using C = std::complex<double>;
  const double zr = z.real(), zi = z.imag(), norm_z = zr * zr + zi * zi;
  if (norm_z < 18.0 * 18.0) {
    if (norm_z == 0.0) {
      log_scale = C(0.0, 0.0);
      return nu == 0.0 ? C(1.0, 0.0) : C(0.0, 0.0);
    }
    const double qr = 0.25 * (zr * zr - zi * zi), qi = 0.5 * zr * zi;  // z^2 / 4
    double tr = inverse_gamma, ti = 0.0, sr = tr, si = 0.0;
    for (int m = 0; m < 400; ++m) {
      const double scale = 1.0 / (static_cast<double>(m + 1) * (nu + static_cast<double>(m) + 1.0));
      const double nr = (tr * qr - ti * qi) * scale, ni = (tr * qi + ti * qr) * scale;
      tr = nr;
      ti = ni;
      sr += tr;
      si += ti;
      if (tr * tr + ti * ti < 1e-36 * (sr * sr + si * si)) break;
    }
    log_scale = nu * std::log(0.5 * z);
    return C(sr, si);
  }
  const double mu = 4.0 * nu * nu;
  const double wr = zr / norm_z, wi = -zi / norm_z;  // 1 / z
  double tr = 1.0, ti = 0.0, sr = 1.0, si = 0.0, previous = 1e300;
  for (int k = 1; k < 200; ++k) {
    const double odd = static_cast<double>(2 * k - 1);
    const double coeff = -(mu - odd * odd) / (static_cast<double>(k) * 8.0);
    const double nr = coeff * (tr * wr - ti * wi), ni = coeff * (tr * wi + ti * wr);
    const double size = nr * nr + ni * ni;
    if (size > previous) break;  // the asymptotic series has started to diverge
    tr = nr;
    ti = ni;
    sr += tr;
    si += ti;
    previous = size;
    if (size < 1e-34 * (sr * sr + si * si)) break;
  }
  log_scale = z - 0.5 * std::log(2.0 * 3.14159265358979323846 * z);
  return C(sr, si);
}

inline std::complex<double> bessel_i_parts(double nu, std::complex<double> z, std::complex<double>& log_scale) {
  return bessel_i_parts(nu, z, log_scale, 1.0 / std::tgamma(nu + 1.0));
}

// Per-order tables for repeated evaluation at one order: 1/Gamma(nu + 1), the series scales 1/((m+1)(nu+m+1)) and the asymptotic coefficients.
struct BesselTables {
  double nu = 0.0, inverse_gamma = 1.0;
  std::vector<double> series_scale, asymptotic;  // size 400 and 200
  explicit BesselTables(double order) : nu(order), inverse_gamma(1.0 / std::tgamma(order + 1.0)), series_scale(400), asymptotic(200) {
    for (int m = 0; m < 400; ++m) series_scale[static_cast<std::size_t>(m)] = 1.0 / (static_cast<double>(m + 1) * (order + static_cast<double>(m) + 1.0));
    const double mu = 4.0 * order * order;
    for (int k = 1; k <= 200; ++k) {
      const double odd = static_cast<double>(2 * k - 1);
      asymptotic[static_cast<std::size_t>(k - 1)] = -(mu - odd * odd) / (static_cast<double>(k) * 8.0);
    }
  }
};

// The same as bessel_i_parts, for a caller that already knows log z (so no complex logarithm is taken) and holds the order's tables.
inline std::complex<double> bessel_i_parts_logz(const BesselTables& t, std::complex<double> z, std::complex<double> log_z, std::complex<double>& log_scale) {
  using C = std::complex<double>;
  const double zr = z.real(), zi = z.imag(), norm_z = zr * zr + zi * zi;
  if (norm_z < 18.0 * 18.0) {
    if (norm_z == 0.0) {
      log_scale = C(0.0, 0.0);
      return t.nu == 0.0 ? C(1.0, 0.0) : C(0.0, 0.0);
    }
    const double qr = 0.25 * (zr * zr - zi * zi), qi = 0.5 * zr * zi;
    double tr = t.inverse_gamma, ti = 0.0, sr = tr, si = 0.0;
    const double* scale = t.series_scale.data();
    for (int m = 0; m < 400; ++m) {
      const double nr = (tr * qr - ti * qi) * scale[m], ni = (tr * qi + ti * qr) * scale[m];
      tr = nr;
      ti = ni;
      sr += tr;
      si += ti;
      if (tr * tr + ti * ti < 1e-36 * (sr * sr + si * si)) break;
    }
    log_scale = t.nu * (log_z - 0.69314718055994530942);
    return C(sr, si);
  }
  const double wr = zr / norm_z, wi = -zi / norm_z;
  double tr = 1.0, ti = 0.0, sr = 1.0, si = 0.0, previous = 1e300;
  const double* coeff = t.asymptotic.data();
  for (int k = 0; k < 200; ++k) {
    const double nr = coeff[k] * (tr * wr - ti * wi), ni = coeff[k] * (tr * wi + ti * wr);
    const double size = nr * nr + ni * ni;
    if (size > previous) break;
    tr = nr;
    ti = ni;
    sr += tr;
    si += ti;
    previous = size;
    if (size < 1e-34 * (sr * sr + si * si)) break;
  }
  log_scale = z - 0.5 * (log_z + 1.8378770664093454836);  // z - (1/2) log(2 pi z)
  return C(sr, si);
}

inline std::complex<double> bessel_i_scaled(double nu, std::complex<double> z) {
  std::complex<double> log_scale;
  const std::complex<double> value = bessel_i_parts(nu, z, log_scale);
  return std::exp(log_scale - z) * value;
}

}  // namespace pluss::subsp::math::finance::internal
