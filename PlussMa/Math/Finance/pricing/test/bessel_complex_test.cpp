// exp(-z) I_nu(z) for complex z against an independent integral representation (Schlaefli):
//   I_nu(z) = (1/pi) int_0^pi e^{z cos t} cos(nu t) dt - (sin(nu pi)/pi) int_0^inf e^{-z cosh t - nu t} dt.
#include "../bessel_complex_internal.h"

#include <cmath>
#include <complex>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using C = std::complex<double>;

void gauss_legendre(int n, std::vector<double>& x, std::vector<double>& w) {
  x.assign(static_cast<std::size_t>(n), 0.0);
  w.assign(static_cast<std::size_t>(n), 0.0);
  for (int i = 0; i < (n + 1) / 2; ++i) {
    double z = std::cos(std::numbers::pi * (i + 0.75) / (n + 0.5)), dp = 1.0;
    for (int it = 0; it < 100; ++it) {
      double p0 = 1.0, p1 = z;
      for (int k = 2; k <= n; ++k) {
        const double p2 = ((2.0 * k - 1.0) * z * p1 - (k - 1.0) * p0) / k;
        p0 = p1;
        p1 = p2;
      }
      dp = n * (z * p1 - p0) / (z * z - 1.0);
      const double dz = p1 / dp;
      z -= dz;
      if (std::abs(dz) < 1e-16) break;
    }
    x[static_cast<std::size_t>(i)] = -z;
    x[static_cast<std::size_t>(n - 1 - i)] = z;
    w[static_cast<std::size_t>(i)] = w[static_cast<std::size_t>(n - 1 - i)] = 2.0 / ((1.0 - z * z) * dp * dp);
  }
}

// exp(-z) I_nu(z) by the integral representation, composite Gauss-Legendre (panels sized to the oscillation).
C reference(double nu, C z) {
  std::vector<double> x, w;
  gauss_legendre(48, x, w);
  const int panels = 8 + static_cast<int>(std::abs(z) / 2.0);
  C first(0.0, 0.0);
  for (int p = 0; p < panels; ++p) {
    const double lo = std::numbers::pi * p / panels, hi = std::numbers::pi * (p + 1) / panels;
    for (std::size_t i = 0; i < x.size(); ++i) {
      const double t = 0.5 * (hi - lo) * x[i] + 0.5 * (hi + lo);
      first += 0.5 * (hi - lo) * w[i] * std::exp(z * (std::cos(t) - 1.0)) * std::cos(nu * t);
    }
  }
  first /= std::numbers::pi;
  C second(0.0, 0.0);
  const double top = 40.0;  // e^{-z (cosh t + 1)} is negligible beyond this for Re z >= 0.5; for small z use the series branch of the test
  for (int p = 0; p < 16; ++p) {
    const double lo = top * p / 16, hi = top * (p + 1) / 16;
    for (std::size_t i = 0; i < x.size(); ++i) {
      const double t = 0.5 * (hi - lo) * x[i] + 0.5 * (hi + lo);
      second += 0.5 * (hi - lo) * w[i] * std::exp(-z * (std::cosh(t) + 1.0) - nu * t);
    }
  }
  return first - std::sin(nu * std::numbers::pi) / std::numbers::pi * second;
}

}  // namespace

int main() {
  try {
    using pluss::subsp::math::finance::internal::bessel_i_scaled;
    // real arguments: closed forms for half-integer orders, I_{1/2}(x) = sqrt(2/(pi x)) sinh x
    for (const double x : {0.3, 2.0, 11.0, 17.9, 18.1, 60.0, 400.0}) {
      const double want = std::sqrt(2.0 / (std::numbers::pi * x)) * 0.5 * (1.0 - std::exp(-2.0 * x));
      const double got = bessel_i_scaled(0.5, C(x, 0.0)).real();
      if (std::abs(got - want) > 1e-12 * std::abs(want)) throw std::runtime_error("I_{1/2} at " + std::to_string(x) + ": " + std::to_string(got) + " vs " + std::to_string(want));
    }
    double worst = 0.0;
    for (const double nu : {-0.9, -0.47, 0.0, 0.6, 1.5, 3.2})
      for (const double mod : {0.5, 3.0, 9.0, 16.0, 17.9, 18.1, 25.0, 80.0, 250.0})
        for (const double arg : {0.0, 0.3, 0.6, 0.78}) {
          const C z = std::polar(mod, arg);
          const C want = reference(nu, z), got = bessel_i_scaled(nu, z);
          const double err = std::abs(got - want) / std::max(std::abs(want), 1e-300);
          worst = std::max(worst, err);
          if (err > 5e-10) throw std::runtime_error("nu " + std::to_string(nu) + " |z| " + std::to_string(mod) + " arg " + std::to_string(arg) + ": " +
                                                    std::to_string(got.real()) + "+" + std::to_string(got.imag()) + "i vs " + std::to_string(want.real()) + "+" +
                                                    std::to_string(want.imag()) + "i (rel " + std::to_string(err) + ")");
        }
    std::cout << "bessel_complex_test passed (worst relative error " << worst << ")\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "bessel_complex_test failed: " << e.what() << '\n';
    return 1;
  }
}
