// Haemodynamics: complex Bessel functions, Womersley flow, Windkessel loads and pulse-wave speed against mpmath/scipy and exact limits.
#include "pluss/subsp/natsci/biology/biomechanics/hemodynamics.h"
#include "hemo_reference.h"

#include <cmath>
#include <complex>
#include <iostream>
#include <stdexcept>
#include <string>

namespace hd = pluss::subsp::natsci::biology::biomechanics::hemodynamics;
namespace ref = hemo_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double cabs_diff(const hd::Complex &a, std::size_t i, double re, double im) {
  const double ar = a.re.contiguous().to_vec<double>()[i], ai = a.im.contiguous().to_vec<double>()[i];
  return std::hypot(ar - re, ai - im) / (1e-300 + std::hypot(re, im));
}
} // namespace

int main() {
  try {
    // ---- complex Bessel J0, J1 by Miller's recurrence vs mpmath (|z| from 1e-3 to 30, all in one batched call)
    {
      const hd::J01 b = hd::bessel_j01({vec(ref::zs_re), vec(ref::zs_im)});
      for (std::size_t i = 0; i < ref::zs_re.size(); ++i) {
        const double e0 = cabs_diff(b.j0, i, ref::bessel[4 * i], ref::bessel[4 * i + 1]), e1 = cabs_diff(b.j1, i, ref::bessel[4 * i + 2], ref::bessel[4 * i + 3]);
        expect(e0 < 1e-12 && e1 < 1e-12, "J0/J1 at z = " + std::to_string(ref::zs_re[i]) + " + " + std::to_string(ref::zs_im[i]) + "i: relative errors " + std::to_string(e0) + ", " + std::to_string(e1));
      }
    }
    // ---- Womersley flow, wall shear and a velocity point at four frequencies (alpha = 4.7 .. 64)
    {
      const std::size_t n = ref::womersley.size() / 8;
      std::vector<double> w, alpha_ref;
      for (std::size_t i = 0; i < n; ++i) { w.push_back(ref::womersley[8 * i]); alpha_ref.push_back(ref::womersley[8 * i + 1]); }
      const Series omega = vec(w), radius = omega.mul(0.0).add(ref::radius), grad = omega.mul(0.0).add(ref::gradient);
      const Series alpha = hd::womersley_number(radius, omega, ref::rho, ref::mu);
      for (std::size_t i = 0; i < n; ++i)
        expect(std::abs(alpha.contiguous().to_vec<double>()[i] - alpha_ref[i]) < 1e-9 * alpha_ref[i], "Womersley number");
      const hd::Complex q = hd::womersley_flow(radius, alpha, omega, ref::rho, grad);
      const hd::Complex tau = hd::womersley_wall_shear(radius, alpha, omega, ref::rho, ref::mu, grad);
      const hd::Complex u = hd::womersley_velocity(omega.mul(0.0).add(0.5), alpha, omega, ref::rho, grad);
      for (std::size_t i = 0; i < n; ++i) {
        expect(cabs_diff(q, i, ref::womersley[8 * i + 2], ref::womersley[8 * i + 3]) < 1e-10, "Womersley flow amplitude, alpha = " + std::to_string(alpha_ref[i]));
        expect(cabs_diff(tau, i, ref::womersley[8 * i + 4], ref::womersley[8 * i + 5]) < 1e-10, "Womersley wall shear, alpha = " + std::to_string(alpha_ref[i]));
        expect(cabs_diff(u, i, ref::womersley[8 * i + 6], ref::womersley[8 * i + 7]) < 1e-10, "Womersley velocity at r/R = 0.5, alpha = " + std::to_string(alpha_ref[i]));
      }
      // momentum equation: i rho w u = A + mu (u'' + u'/r) at interior points (central differences in r)
      const double h = 1e-4;
      for (const double s : {0.3, 0.6, 0.85}) {
        const auto at = [&](double rs) { return hd::womersley_velocity(omega.mul(0.0).add(rs), alpha, omega, ref::rho, grad); };
        const hd::Complex um = at(s - h), u0 = at(s), up = at(s + h);
        for (std::size_t i = 0; i < n; ++i) {
          const auto g = [&](const hd::Complex &c) { return std::complex<double>(c.re.contiguous().to_vec<double>()[i], c.im.contiguous().to_vec<double>()[i]); };
          const double r = s * ref::radius, dr = h * ref::radius;
          const std::complex<double> upp = (g(up) - 2.0 * g(u0) + g(um)) / (dr * dr), up1 = (g(up) - g(um)) / (2.0 * dr);
          const std::complex<double> lhs = std::complex<double>(0.0, 1.0) * ref::rho * w[i] * g(u0), rhs = ref::gradient + ref::mu * (upp + up1 / r);
          expect(std::abs(lhs - rhs) < 2e-4 * std::abs(lhs), "momentum equation residual at r/R = " + std::to_string(s) + ", alpha = " + std::to_string(alpha_ref[i]) + ": " + std::to_string(std::abs(lhs - rhs) / std::abs(lhs)));
        }
        // no slip at the wall
      }
      const hd::Complex wall = hd::womersley_velocity(omega.mul(0.0).add(1.0), alpha, omega, ref::rho, grad);
      expect(wall.abs().max().item<double>() < 1e-9 * ref::gradient / (w[0] * ref::rho), "no-slip at the wall");
      // force balance on the fluid column: pressure force (pi R^2 A) = wall shear (2 pi R tau_w) + inertia (i rho w Q): check the quasi-steady identity
      for (std::size_t i = 0; i < n; ++i) {
        const std::complex<double> qq(q.re.contiguous().to_vec<double>()[i], q.im.contiguous().to_vec<double>()[i]), tt(tau.re.contiguous().to_vec<double>()[i], tau.im.contiguous().to_vec<double>()[i]);
        const std::complex<double> lhs = M_PI * ref::radius * ref::radius * ref::gradient;
        const std::complex<double> rhs = 2.0 * M_PI * ref::radius * tt + std::complex<double>(0.0, 1.0) * ref::rho * w[i] * qq;
        expect(std::abs(lhs - rhs) < 1e-9 * std::abs(lhs), "axial momentum balance of the fluid column, alpha = " + std::to_string(alpha_ref[i]));
      }
    }
    // ---- Poiseuille limit: alpha -> 0 gives Q = pi R^4 A / (8 mu) in phase with the gradient, and R_p = 8 mu L / (pi R^4)
    {
      const Series omega = vec({1e-4}), radius = vec({ref::radius}), grad = vec({ref::gradient});
      const Series alpha = hd::womersley_number(radius, omega, ref::rho, ref::mu);
      const hd::Complex q = hd::womersley_flow(radius, alpha, omega, ref::rho, grad);
      const double poiseuille = M_PI * std::pow(ref::radius, 4.0) * ref::gradient / (8.0 * ref::mu);
      expect(std::abs(q.re.item<double>() - poiseuille) < 1e-4 * poiseuille && std::abs(q.im.item<double>()) < 1e-3 * poiseuille, "Poiseuille limit of Womersley flow");
      const hd::Complex z = hd::womersley_impedance(radius, alpha, omega, ref::rho, 0.5);
      expect(std::abs(z.re.item<double>() - hd::poiseuille_resistance(ref::mu, 0.5, radius).item<double>()) < 1e-4 * z.re.item<double>(), "low-frequency impedance = Poiseuille resistance");
    }
    // ---- 3-element Windkessel: steady-cycle pressure from the impedance harmonics vs the scipy ODE solution; exact exponential integrator
    {
      const double rp = ref::windkessel[0], c = ref::windkessel[1], zc = ref::windkessel[2], period = ref::windkessel[3], q0 = ref::windkessel[4];
      const double w0 = 2.0 * M_PI / period;
      const std::size_t nt = 8;
      const std::size_t base = 5 + 6;
      std::vector<double> tgrid, pref;
      for (std::size_t i = 0; i < nt; ++i) { tgrid.push_back(ref::windkessel[base + i]); pref.push_back(ref::windkessel[base + nt + i]); }
      std::vector<double> pred;
      const Series k = vec({1.0, 2.0, 3.0});
      const hd::Complex z = hd::windkessel3_impedance(k.mul(w0), zc, rp, c);
      const auto zr = z.re.contiguous().to_vec<double>(), zi = z.im.contiguous().to_vec<double>();
      for (const double t : tgrid) {
        double p = (rp + zc) * q0;
        for (int h = 0; h < 3; ++h) {
          const double amp = ref::windkessel[5 + 2 * h], ph = ref::windkessel[5 + 2 * h + 1];
          const std::complex<double> zz(zr[static_cast<std::size_t>(h)], zi[static_cast<std::size_t>(h)]);
          p += std::real(zz * amp * std::exp(std::complex<double>(0.0, (h + 1) * w0 * t + ph)));
        }
        pred.push_back(p);
      }
      for (std::size_t i = 0; i < nt; ++i)
        expect(std::abs(pred[i] - pref[i]) < 1e-4 * (1.0 + std::abs(pref[i])) + 1.0, "Windkessel steady cycle: impedance sum " + std::to_string(pred[i]) + " vs ODE " + std::to_string(pref[i]));
      // exponential integrator over many cycles reaches the same cycle (piecewise-constant flow over a very fine step)
      Series pc = vec({0.0});
      const auto flow = [&](double t) { double q = q0; for (int h = 0; h < 3; ++h) q += ref::windkessel[5 + 2 * h] * std::cos((h + 1) * w0 * t + ref::windkessel[5 + 2 * h + 1]); return q; };
      const double dt = period / 4000.0;
      double t = 0.0;
      const double t_end = 39.0 * period;
      while (t < t_end - 1e-12) {
        pc = hd::windkessel_capacitor_step(pc, vec({flow(t + 0.5 * dt)}), dt, rp, c);
        t += dt;
      }
      const double p_end = hd::windkessel3_pressure(pc, vec({flow(t)}), zc).item<double>();
      expect(std::abs(p_end - pref[0]) < 2e-3 * pref[0], "exponential integrator reaches the same cardiac cycle: " + std::to_string(p_end) + " vs " + std::to_string(pref[0]));
      // diastolic decay P = P0 exp(-t / (R C)) with zero flow, exactly
      const Series decayed = hd::windkessel_capacitor_step(vec({90.0}), vec({0.0}), 0.4, 1.1, 1.3);
      expect(std::abs(decayed.item<double>() - 90.0 * std::exp(-0.4 / (1.1 * 1.3))) < 1e-12, "diastolic decay");
      // 2-element impedance limits: Z(0) = R, |Z| -> 1 / (w C) at high frequency
      const hd::Complex z2 = hd::windkessel2_impedance(vec({0.0, 1e6}), 1.1, 1.3);
      expect(std::abs(z2.re.contiguous().to_vec<double>()[0] - 1.1) < 1e-14 && std::abs(z2.abs().contiguous().to_vec<double>()[1] * 1e6 * 1.3 - 1.0) < 1e-6, "2-element impedance limits");
    }
    // ---- pulse-wave speed: Bramwell-Hill with dA/dP of a thin elastic tube equals Moens-Korteweg
    {
      const double e = 4.0e5, hh = 1.0e-3, rho = 1050.0;
      const Series r = vec({0.004, 0.012});
      const Series area = r.square().mul(M_PI), da_dp = r.pow(3.0).mul(2.0 * M_PI / (e * hh)); // dR/dP = R^2 / (E h)
      const Series bh = hd::bramwell_hill(area, rho, da_dp), mk = hd::moens_korteweg(e, hh, rho, r);
      expect(bh.sub(mk).abs().max().item<double>() < 1e-12, "Bramwell-Hill = Moens-Korteweg for a thin elastic tube");
      expect(std::abs(mk.select(0, 0).item<double>() - std::sqrt(e * hh / (2.0 * rho * 0.004))) < 1e-12, "Moens-Korteweg closed form");
    }
    std::cout << "hemodynamics tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
