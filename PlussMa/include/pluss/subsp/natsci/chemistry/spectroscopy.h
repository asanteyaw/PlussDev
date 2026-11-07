#pragma once

// Absorption spectroscopy and molecular statistical mechanics, batched over lanes.
//
//  * Beer-Lambert: A = l sum_i eps_i c_i (decadic), transmittance 10^-A.
//  * Line shapes: Gaussian, Lorentzian and Voigt profiles. The Voigt profile is
//      V(x) = Re w(z) / (sigma sqrt(2 pi)),  z = (x + i gamma)/(sigma sqrt 2),
//    with the Faddeeva function w from the rational approximation of J. A. C. Weideman,
//    "Computation of the complex error function", SIAM J. Numer. Anal. 31 (1994) 1497
//    (N = 32 terms, ~1e-13 accuracy; coefficients computed once by a discrete Fourier sum, real/imaginary
//    pair arithmetic so no complex Series dtype is needed). Doppler width sigma = (nu0/c) sqrt(kT/m).
//  * Diatomic levels: G(v) = we (v + 1/2) - wexe (v + 1/2)^2, B_v = Be - alphae (v + 1/2),
//    F(J) = B_v J (J + 1); partition functions and the internal heat capacity
//    cv/R = Var(E)/(kT)^2 summed over the level ladder (exact for the model, no derivatives).
// Wavenumbers in cm^-1 unless a function says SI.

#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>
#include <numbers>
#include <complex>
#include <vector>

namespace pluss::subsp::natsci::chemistry::spectroscopy {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace constants = ::pluss::subsp::natsci::common::constants::codata2022;

inline double speed_of_light() { return constants::speed_of_light.measurement.value().si_value(); }
inline double planck() { return constants::planck_constant.measurement.value().si_value(); }
inline double boltzmann() { return constants::boltzmann_constant.measurement.value().si_value(); }
inline double avogadro() { return constants::avogadro_constant.measurement.value().si_value(); }

// ---- Beer-Lambert ------------------------------------------------------------------------------------
// absorptivity [S] or [..., S], concentration [..., S], path length scalar or [...].
inline Series absorbance(const Series &absorptivity, const Series &concentration, const dynamics::Coeff &path_length) {
  return absorptivity.mul(concentration).sum(-1, false).mul(path_length.on(concentration));
}
inline Series transmittance(const Series &a) { return a.mul(-std::log(10.0)).exp(); }

// ---- Line shapes -------------------------------------------------------------------------------------
inline Series gaussian(const Series &x, const Series &x0, const Series &sigma) {
  const Series u = x.sub(x0).div(sigma);
  return u.square().mul(-0.5).exp().div(sigma.mul(std::sqrt(2.0 * std::numbers::pi_v<double>)));
}
inline Series lorentzian(const Series &x, const Series &x0, const Series &gamma) {
  const Series d = x.sub(x0);
  return gamma.div(d.square().add(gamma.square())).mul(1.0 / std::numbers::pi_v<double>);
}

namespace detail {
struct Weideman final {
  static constexpr int n = 32;
  double l{0.0};
  std::vector<double> a; // a_1 .. a_N (a[0] = a_1)
  Weideman() {
    const int m = 2 * n, m2 = 2 * m;
    l = std::sqrt(static_cast<double>(n) / std::sqrt(2.0));
    std::vector<double> f(static_cast<std::size_t>(m2), 0.0); // f(0) = 0 prepended to the 2M-1 samples
    for (int idx = 0; idx < 2 * m - 1; ++idx) {
      const int k = -m + 1 + idx;
      const double theta = static_cast<double>(k) * std::numbers::pi_v<double> / m;
      const double t = l * std::tan(theta / 2.0);
      f[static_cast<std::size_t>(idx + 1)] = std::exp(-t * t) * (l * l + t * t);
    }
    // fftshift of an even-length vector, then the DFT: a_n = Re sum_j fs[j] exp(-2 pi i n j / m2) / m2
    std::vector<double> fs(static_cast<std::size_t>(m2));
    for (int j = 0; j < m2; ++j)
      fs[static_cast<std::size_t>(j)] = f[static_cast<std::size_t>((j + m2 / 2) % m2)];
    a.resize(static_cast<std::size_t>(n));
    for (int nn = 1; nn <= n; ++nn) {
      double acc = 0.0;
      for (int j = 0; j < m2; ++j)
        acc += fs[static_cast<std::size_t>(j)] * std::cos(2.0 * std::numbers::pi_v<double> * nn * j / m2);
      a[static_cast<std::size_t>(nn - 1)] = acc / m2;
    }
  }
};
inline const Weideman &weideman() {
  static const Weideman w;
  return w;
}
} // namespace detail

struct Faddeeva final {
  Series real = pluss::series::undefined();
  Series imag = pluss::series::undefined();
};

// w(z) = exp(-z^2) erfc(-i z) for z = x + i y with y >= 0, elementwise (x and y broadcast together).
inline Faddeeva faddeeva(const Series &x, const Series &y) {
  dynamics::require(y.ge(0.0).all().item<bool>(), "faddeeva: imaginary part must be non-negative");
  const auto &w = detail::weideman();
  const double l = w.l;
  const Series u = y.add(l);                       // L + y
  const Series d = u.square().add(x.square());     // |L - i z|^2 = u^2 + x^2
  const Series zr = y.neg().add(l).mul(u).sub(x.square()).div(d); // Re Z, Z = (L + i z)/(L - i z)
  const Series zi = x.mul(2.0 * l).div(d);                           // Im Z
  // Horner: p(Z) = sum_{n=1..N} a_n Z^(n-1)
  Series pr = x.mul(0.0).add(w.a.back()), pi = x.mul(0.0);
  for (int idx = detail::Weideman::n - 2; idx >= 0; --idx) {
    const Series nr = pr.mul(zr).sub(pi.mul(zi)).add(w.a[static_cast<std::size_t>(idx)]);
    const Series ni = pr.mul(zi).add(pi.mul(zr));
    pr = nr;
    pi = ni;
  }
  // 1/(L - i z) = (u + i x)/d ;  1/(L - i z)^2 = ((u^2 - x^2) + 2 i u x)/d^2
  const Series d2 = d.square();
  const Series inv1r = u.div(d), inv1i = x.div(d);
  const Series inv2r = u.square().sub(x.square()).div(d2), inv2i = u.mul(x).mul(2.0).div(d2);
  const double c = 1.0 / std::sqrt(std::numbers::pi_v<double>);
  return {pr.mul(inv2r).sub(pi.mul(inv2i)).mul(2.0).add(inv1r.mul(c)),
          pr.mul(inv2i).add(pi.mul(inv2r)).mul(2.0).add(inv1i.mul(c))};
}

// Voigt profile (area 1): Gaussian standard deviation sigma, Lorentzian half width gamma.
inline Series voigt(const Series &x, const Series &x0, const Series &sigma, const Series &gamma) {
  dynamics::require_positive(sigma, "voigt: sigma");
  dynamics::require_nonnegative(gamma, "voigt: gamma");
  const double s2 = std::sqrt(2.0);
  const Series xr = x.sub(x0).div(sigma.mul(s2)), yr = gamma.div(sigma.mul(s2));
  return faddeeva(xr, yr).real.div(sigma.mul(std::sqrt(2.0 * std::numbers::pi_v<double>)));
}

// Olivero & Longbothum (1977) approximation of the Voigt FWHM from the component FWHMs.
inline Series voigt_fwhm_approx(const Series &lorentz_fwhm, const Series &gauss_fwhm) {
  return lorentz_fwhm.mul(0.5346).add(lorentz_fwhm.square().mul(0.2166).add(gauss_fwhm.square()).sqrt());
}

// Doppler standard deviation (same unit as the line centre nu0): sigma = (nu0/c) sqrt(kT/m), M in kg/mol.
inline Series doppler_sigma(const Series &nu0, const Series &temperature, double molar_mass) {
  const double m = molar_mass / avogadro();
  return nu0.mul(temperature.mul(boltzmann() / m).sqrt()).div(speed_of_light());
}

// ---- Diatomic energy ladder -------------------------------------------------------------------------
struct Diatomic final {
  double we{0.0}, wexe{0.0}, be{0.0}, alphae{0.0};
  double symmetry{1.0};            // 2 for homonuclear (statistical, high-T limit)
  double electronic_degeneracy{1.0};
};

inline double wavenumber_to_energy(double wavenumber_cm) { return planck() * speed_of_light() * wavenumber_cm * 100.0; }

namespace detail {
inline Series ladder(const std::vector<double> &energies_cm, const Series &like) {
  return pluss::make_series(energies_cm, {static_cast<std::int64_t>(energies_cm.size())})
      .to(like.scalar_type()).to(like.platform());
}
// ln-sum-exp moments of a level ladder (energies in J, degeneracies g) at temperature [...]: mean and variance
// of E in units of kT, plus the partition function.
struct Moments final {
  Series q = pluss::series::undefined();      // partition function
  Series mean = pluss::series::undefined();   // <E>/kT
  Series var = pluss::series::undefined();    // Var(E)/(kT)^2 = cv/R of this degree of freedom
};
inline Moments moments(const Series &energy_j, const Series &degeneracy, const Series &t) {
  const Series x = energy_j.div(t.unsqueeze(-1).mul(boltzmann()));           // [..., levels]
  const Series w = degeneracy.mul(x.neg().exp());
  const Series q = w.sum(-1, false);
  const Series p = w.div(q.unsqueeze(-1));
  const Series mean = p.mul(x).sum(-1, false);
  const Series var = p.mul(x.square()).sum(-1, false).sub(mean.square());
  return {q, mean, var};
}
} // namespace detail

// Rotational ladder of vibrational level v.
inline detail::Moments rotational(const Diatomic &d, const Series &t, int v = 0, int jmax = 200) {
  const double bv = d.be - d.alphae * (v + 0.5);
  std::vector<double> e, g;
  for (int j = 0; j <= jmax; ++j) {
    e.push_back(bv * j * (j + 1));
    g.push_back((2.0 * j + 1.0) / d.symmetry);
  }
  const Series energy = detail::ladder(e, t).mul(planck() * speed_of_light() * 100.0);
  return detail::moments(energy, detail::ladder(g, t), t);
}

// Vibrational ladder (anharmonic G(v) measured from the zero-point level).
inline detail::Moments vibrational(const Diatomic &d, const Series &t, int vmax = 60) {
  std::vector<double> e, g;
  const auto gv = [&](int v) { return d.we * (v + 0.5) - d.wexe * (v + 0.5) * (v + 0.5); };
  for (int v = 0; v <= vmax; ++v) {
    // truncate at the dissociation turning point where G(v) stops increasing
    if (v > 0 && gv(v) < gv(v - 1))
      break;
    e.push_back(gv(v) - gv(0));
    g.push_back(1.0);
  }
  const Series energy = detail::ladder(e, t).mul(planck() * speed_of_light() * 100.0);
  return detail::moments(energy, detail::ladder(g, t), t);
}

// Ideal-gas isobaric heat capacity cp/R of a diatomic from translation (3/2) + PV (1) + rotation + vibration.
inline Series cp_over_r(const Diatomic &d, const Series &t) {
  return rotational(d, t).var.add(vibrational(d, t).var).add(2.5);
}

} // namespace pluss::subsp::natsci::chemistry::spectroscopy
