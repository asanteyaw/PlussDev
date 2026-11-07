#pragma once

// Arterial haemodynamics, batched over lanes: lumped Windkessel loads, Poiseuille resistance, Womersley pulsatile flow in a rigid tube, and
// pulse-wave speed.
//
//  * Windkessel (Frank 1899; Westerhof, Lankhaar & Westerhof 2009 for the 3-element form; neither obtained, the equations are verified against an
//    independent ODE solve and the exact impedance):
//      2-element  C dP/dt = Q - P/R,                impedance Z(w) = R / (1 + i w R C)
//      3-element  P = Zc Q + Pc,  C dPc/dt = Q - Pc/R,   Z(w) = Zc + R / (1 + i w R C)
//    `windkessel3_step` advances Pc exactly over a step with piecewise-constant flow (exponential integrator), so a stiff small RC needs no tiny step.
//  * Poiseuille: R_p = 8 mu L / (pi r^4).
//  * Womersley (1955; not obtained): fluid of density rho and viscosity mu in a rigid tube of radius R under the pressure gradient
//    -dp/dz = A e^{i w t}, alpha = R sqrt(w rho / mu), Lambda = i^{3/2} alpha:
//      u(r, t) = Re{ (A / (i rho w)) [1 - J0(Lambda r/R) / J0(Lambda)] e^{i w t} }
//      Q       = (pi R^2 A / (i rho w)) [1 - 2 J1(Lambda) / (Lambda J0(Lambda))]                   (complex amplitude)
//      tau_w   = -mu du/dr |_R = i mu A Lambda J1(Lambda) / (rho w R J0(Lambda))
//    verified against mpmath's complex Bessel functions, by substitution into the momentum equation i rho w u = A + mu (u'' + u'/r), the axial
//    force balance pi R^2 A = 2 pi R tau_w + i rho w Q, and the Poiseuille limit alpha -> 0 (an earlier draft carried the wrong sign of i; only the
//    substitution test, which does not share the formula, exposed it).
//  * Pulse-wave speed: Moens-Korteweg c0 = sqrt(E h / (2 rho R)) (thin elastic tube) and Bramwell-Hill c = sqrt(A / (rho dA/dP)).
//
// Complex numbers are carried as separate real and imaginary Series (struct Complex).

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <numbers>

namespace pluss::subsp::natsci::biology::biomechanics::hemodynamics {

using dynamics::Coeff;

struct Complex final {
  Series re = pluss::series::undefined(), im = pluss::series::undefined();
  [[nodiscard]] Series abs() const { return re.square().add(im.square()).sqrt(); }
  [[nodiscard]] Series arg() const { return im.arctan2(re); }
};
inline Complex cmul(const Complex &a, const Complex &b) { return {a.re.mul(b.re).sub(a.im.mul(b.im)), a.re.mul(b.im).add(a.im.mul(b.re))}; }
// Smith's algorithm: no overflow/underflow from squaring the denominator
inline Complex cdiv(const Complex &a, const Complex &b) {
  const Series use_re = b.re.abs().ge(b.im.abs());
  const Series safe_re = use_re.where(b.re, b.re.mul(0.0).add(1.0)), safe_im = use_re.where(b.im.mul(0.0).add(1.0), b.im);
  const Series r1 = b.im.div(safe_re), den1 = b.re.add(b.im.mul(r1));
  const Series r2 = b.re.div(safe_im), den2 = b.re.mul(r2).add(b.im);
  const Series re = use_re.where(a.re.add(a.im.mul(r1)).div(den1), a.re.mul(r2).add(a.im).div(den2));
  const Series im = use_re.where(a.im.sub(a.re.mul(r1)).div(den1), a.im.mul(r2).sub(a.re).div(den2));
  return {re, im};
}
inline Complex csub(const Complex &a, const Complex &b) { return {a.re.sub(b.re), a.im.sub(b.im)}; }
inline Complex cscale(const Complex &a, const Series &s) { return {a.re.mul(s), a.im.mul(s)}; }
inline Complex cexp(const Complex &a) { const Series e = a.re.exp(); return {e.mul(a.im.cos()), e.mul(a.im.sin())}; }

// ---- complex Bessel functions J0, J1 by Miller's backward recurrence (stable for every complex argument) ------------------------
struct J01 final {
  Complex j0, j1;
};
inline J01 bessel_j01(const Complex &z) {
  const Series mod = z.abs();
  const int top = static_cast<int>(std::ceil(1.2 * mod.max().item<double>())) + 60;
  Complex next{z.re.mul(0.0), z.re.mul(0.0)};             // J_{k+1}
  Complex cur{z.re.mul(0.0).add(1e-30), z.re.mul(0.0)};   // J_k, started at k = top (any scale: normalised below)
  Complex sum_even{z.re.mul(0.0), z.re.mul(0.0)};         // J_0 + 2 sum_{k >= 1} J_{2k}            (= 1)
  Complex sum_alt{z.re.mul(0.0), z.re.mul(0.0)};          // J_0 + 2 sum_{k >= 1} (-1)^k J_{2k}     (= cos z)
  Complex j1{z.re.mul(0.0), z.re.mul(0.0)};
  Complex j0{z.re.mul(0.0), z.re.mul(0.0)};
  const Complex inv = cdiv(Complex{z.re.mul(0.0).add(1.0), z.re.mul(0.0)}, z); // 1 / z
  for (int k = top; k >= 1; --k) {
    // J_{k-1} = (2k / z) J_k - J_{k+1}
    const Complex prev = csub(cscale(cmul(inv, cur), z.re.mul(0.0).add(2.0 * k)), next);
    if (k - 1 == 0) {
      j0 = prev;
      j1 = cur;
      sum_even = {sum_even.re.add(prev.re), sum_even.im.add(prev.im)};
      sum_alt = {sum_alt.re.add(prev.re), sum_alt.im.add(prev.im)};
    } else if ((k - 1) % 2 == 0) {
      const double sgn = (((k - 1) / 2) % 2 == 0) ? 1.0 : -1.0;
      sum_even = {sum_even.re.add(prev.re.mul(2.0)), sum_even.im.add(prev.im.mul(2.0))};
      sum_alt = {sum_alt.re.add(prev.re.mul(2.0 * sgn)), sum_alt.im.add(prev.im.mul(2.0 * sgn))};
    }
    next = cur;
    cur = prev;
    // rescale all lanes that grew large (keeps the recurrence inside double range)
    const Series big = cur.re.abs().maximum(cur.im.abs()).gt(1e100);
    const Series s = big.where(cur.re.mul(0.0).add(1e-100), cur.re.mul(0.0).add(1.0));
    cur = cscale(cur, s);
    next = cscale(next, s);
    sum_even = cscale(sum_even, s);
    sum_alt = cscale(sum_alt, s);
    j0 = cscale(j0, s);
    j1 = cscale(j1, s);
  }
  // Normalisation. 1 = J0 + 2 sum J_2k is exact but, for large |Im z|, its terms are ~e^{|Im z|} and cancel to 1 (a relative loss of e^{|Im z|}
  // epsilon); cos z = J0 + 2 sum (-1)^k J_2k has terms and value of the same size, so it is used once |Im z| > 1.
  const Series use_cos = z.im.abs().gt(1.0);
  const Complex cosz{z.re.cos().mul(z.im.cosh()), z.re.sin().mul(z.im.sinh()).neg()};
  const Complex scale_one = cdiv(Complex{z.re.mul(0.0).add(1.0), z.re.mul(0.0)}, sum_even), scale_cos = cdiv(cosz, sum_alt);
  const Complex scale{use_cos.where(scale_cos.re, scale_one.re), use_cos.where(scale_cos.im, scale_one.im)};
  return {cmul(j0, scale), cmul(j1, scale)};
}

// ---- Windkessel ----------------------------------------------------------------------------------------------------------------
// complex input impedance at angular frequency w
inline Complex windkessel2_impedance(const Series &omega, const Coeff &resistance, const Coeff &compliance) {
  const Series r = resistance.on(omega), c = compliance.on(omega);
  return cdiv(Complex{r, r.mul(0.0)}, Complex{omega.mul(0.0).add(1.0), omega.mul(r).mul(c)});
}
inline Complex windkessel3_impedance(const Series &omega, const Coeff &characteristic, const Coeff &resistance, const Coeff &compliance) {
  const Complex z2 = windkessel2_impedance(omega, resistance, compliance);
  return {z2.re.add(characteristic.on(omega)), z2.im};
}
// exact update of the capacitor pressure over dt for a flow q held constant: Pc -> Pc e^{-dt/RC} + R q (1 - e^{-dt/RC})
inline Series windkessel_capacitor_step(const Series &pc, const Series &flow, double dt, const Coeff &resistance, const Coeff &compliance) {
  const Series decay = resistance.on(pc).mul(compliance.on(pc)).reciprocal().mul(-dt).exp();
  return pc.mul(decay).add(resistance.on(pc).mul(flow).mul(decay.neg().add(1.0)));
}
inline Series windkessel3_pressure(const Series &pc, const Series &flow, const Coeff &characteristic) { return pc.add(flow.mul(characteristic.on(pc))); }

// ---- Poiseuille and Womersley ----------------------------------------------------------------------------------------------------
inline Series poiseuille_resistance(const Coeff &viscosity, const Coeff &length, const Series &radius) {
  return radius.pow(4.0).mul(std::numbers::pi_v<double>).reciprocal().mul(8.0).mul(viscosity.on(radius)).mul(length.on(radius));
}
inline Series womersley_number(const Series &radius, const Series &omega, const Coeff &density, const Coeff &viscosity) {
  return radius.mul(omega.mul(density.on(omega)).div(viscosity.on(omega)).sqrt());
}
// Lambda = i^{3/2} alpha = alpha (-1 + i)/sqrt(2)... i^{3/2} = e^{i 3 pi / 4} = (-1 + i)/sqrt(2)
inline Complex womersley_lambda(const Series &alpha) { return {alpha.mul(-std::sqrt(0.5)), alpha.mul(std::sqrt(0.5))}; }

// complex amplitude of the axial velocity at the relative radius s = r/R for the gradient amplitude A (time factor e^{i w t} understood)
inline Complex womersley_velocity(const Series &relative_radius, const Series &alpha, const Series &omega, const Coeff &density, const Series &gradient) {
  const Complex lam = womersley_lambda(alpha);
  const Complex at_wall = bessel_j01(lam).j0;
  const Complex inside = bessel_j01({lam.re.mul(relative_radius).sub(lam.im.mul(0.0)), lam.im.mul(relative_radius)}).j0;
  const Complex ratio = cdiv(inside, at_wall);
  const Series scale = gradient.div(omega.mul(density.on(omega)));
  // (A / (i rho w)) (1 - ratio) = -i (A / (rho w)) (1 - ratio)
  const Complex one_minus{ratio.re.neg().add(1.0), ratio.im.neg()};
  return {one_minus.im.mul(scale), one_minus.re.neg().mul(scale)};
}
inline Complex womersley_flow(const Series &radius, const Series &alpha, const Series &omega, const Coeff &density, const Series &gradient) {
  const Complex lam = womersley_lambda(alpha);
  const J01 b = bessel_j01(lam);
  const Complex term = cscale(cdiv(b.j1, cmul(lam, b.j0)), alpha.mul(0.0).add(2.0));
  const Complex bracket{term.re.neg().add(1.0), term.im.neg()};
  const Series scale = radius.square().mul(std::numbers::pi_v<double>).mul(gradient).div(omega.mul(density.on(omega)));
  return {bracket.im.mul(scale), bracket.re.neg().mul(scale)};
}
inline Complex womersley_wall_shear(const Series &radius, const Series &alpha, const Series &omega, const Coeff &density, const Coeff &viscosity, const Series &gradient) {
  const Complex lam = womersley_lambda(alpha);
  const J01 b = bessel_j01(lam);
  const Complex ratio = cdiv(b.j1, b.j0);
  const Series scale = gradient.div(omega.mul(density.on(omega))).mul(viscosity.on(omega)).div(radius);
  const Complex prod = cmul(lam, ratio);          // Lambda J1 / J0
  const Complex i_prod{prod.im.neg(), prod.re};   // i * (...)
  return cscale(i_prod, scale);
}
// input impedance of a rigid tube segment of length L: pressure drop A L over flow amplitude
inline Complex womersley_impedance(const Series &radius, const Series &alpha, const Series &omega, const Coeff &density, const Coeff &length) {
  const Series one = alpha.mul(0.0).add(1.0);
  const Complex q = womersley_flow(radius, alpha, omega, density, one);
  return cdiv(Complex{length.on(alpha), alpha.mul(0.0)}, q);
}

// ---- pulse-wave speed --------------------------------------------------------------------------------------------------------------
inline Series moens_korteweg(const Coeff &young, const Coeff &thickness, const Coeff &density, const Series &radius) {
  return young.on(radius).mul(thickness.on(radius)).div(density.on(radius).mul(radius).mul(2.0)).sqrt();
}
inline Series bramwell_hill(const Series &area, const Coeff &density, const Series &d_area_d_pressure) {
  return area.div(d_area_d_pressure.mul(density.on(area))).sqrt();
}

} // namespace pluss::subsp::natsci::biology::biomechanics::hemodynamics
