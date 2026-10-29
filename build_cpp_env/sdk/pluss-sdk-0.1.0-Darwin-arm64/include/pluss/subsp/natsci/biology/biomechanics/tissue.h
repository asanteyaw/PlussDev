#pragma once

// Finite-strain mechanics of soft biological tissue: incompressible hyperelastic strain-energy models and the stress of the standard homogeneous
// deformations, batched over stretch lanes and material-parameter lanes.
//
// Isotropic models W(I1, I2) with I1 = l1^2 + l2^2 + l3^2, I2 = l1^-2 + l2^-2 + l3^-2 (l1 l2 l3 = 1); the Cauchy stress follows from
//     sigma_i - sigma_j = l_i dW/dl_i - l_j dW/dl_j,   dW/dl_i = 2 l_i W1 - 2 l_i^-3 W2     (W1 = dW/dI1, W2 = dW/dI2)
//   uniaxial tension       sigma = 2 (l^2 - 1/l) (W1 + W2 / l)
//   equibiaxial tension    sigma = 2 (l^2 - l^-4) (W1 + l^2 W2)
//   planar (pure shear)    sigma_1 - sigma_3 = 2 (l^2 - l^-2) (W1 + W2)
//   simple shear gamma     sigma_12 = 2 gamma (W1 + W2),   sigma_11 - sigma_22 = 2 gamma^2 (W1 + W2)
// Models: neo-Hookean (W = mu/2 (I1-3)), Mooney-Rivlin (c10 (I1-3) + c01 (I2-3)), Yeoh (sum c_k (I1-3)^k, k <= 3), Gent (-mu Jm/2 ln(1 - (I1-3)/Jm)),
// Fung-type exponential (c/2 (exp(b (I1-3)) - 1)); Ogden (sum mu_p/alpha_p (l1^a + l2^a + l3^a - 3)) is handled in principal stretches.
// All reduce to the shear modulus mu0 = 2 (W1 + W2)|_{I1=I2=3} and Young's modulus 3 mu0 at small strain (checked).
//
// Fibre-reinforced arterial wall (Holzapfel-Gasser-Ogden / Gasser-Ogden-Holzapfel dispersion form, as written in the Abaqus Analysis User's Guide,
// "Anisotropic hyperelastic behavior", read; the journal papers could not be fetched):
//     W = C10 (I1 - 3) + k1/(2 k2) sum_{alpha=1,2} (exp(k2 <E_alpha>^2) - 1),   E = kappa (I1 - 3) + (1 - 3 kappa)(I4 - 1),   <x> = (x + |x|)/2
// with two symmetric families a = (cos g, +-sin g, 0) in the circumferential-axial plane, I4 = l_t^2 cos^2 g + l_z^2 sin^2 g; fibres carry tension only.
// `artery_stresses` gives the Cauchy stress differences for circumferential/axial stretches (l_r = 1/(l_t l_z)); the radial stress of a thin wall is 0.

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <functional>

namespace pluss::subsp::natsci::biology::biomechanics::tissue {

using dynamics::Coeff;

// ---- isotropic invariants models ---------------------------------------------------------------------------------------------------
struct Isotropic final {
  std::function<Series(const Series &i1, const Series &i2)> energy, w1, w2;
};

inline Isotropic neo_hookean(Coeff mu) {
  return {[=](const Series &i1, const Series &) { return i1.sub(3.0).mul(mu.on(i1)).mul(0.5); },
          [=](const Series &i1, const Series &) { return i1.mul(0.0).add(0.5).mul(mu.on(i1)); },
          [=](const Series &i1, const Series &) { return i1.mul(0.0); }};
}
inline Isotropic mooney_rivlin(Coeff c10, Coeff c01) {
  return {[=](const Series &i1, const Series &i2) { return i1.sub(3.0).mul(c10.on(i1)).add(i2.sub(3.0).mul(c01.on(i1))); },
          [=](const Series &i1, const Series &) { return i1.mul(0.0).add(1.0).mul(c10.on(i1)); },
          [=](const Series &i1, const Series &) { return i1.mul(0.0).add(1.0).mul(c01.on(i1)); }};
}
inline Isotropic yeoh(Coeff c10, Coeff c20, Coeff c30) {
  return {[=](const Series &i1, const Series &) { const Series x = i1.sub(3.0); return x.mul(c10.on(i1)).add(x.square().mul(c20.on(i1))).add(x.pow(3.0).mul(c30.on(i1))); },
          [=](const Series &i1, const Series &) { const Series x = i1.sub(3.0); return c10.on(i1).add(x.mul(c20.on(i1)).mul(2.0)).add(x.square().mul(c30.on(i1)).mul(3.0)); },
          [=](const Series &i1, const Series &) { return i1.mul(0.0); }};
}
inline Isotropic gent(Coeff mu, Coeff jm) {
  return {[=](const Series &i1, const Series &) { return i1.sub(3.0).div(jm.on(i1)).neg().add(1.0).log().mul(mu.on(i1)).mul(jm.on(i1)).mul(-0.5); },
          [=](const Series &i1, const Series &) { return mu.on(i1).mul(0.5).div(i1.sub(3.0).div(jm.on(i1)).neg().add(1.0)); },
          [=](const Series &i1, const Series &) { return i1.mul(0.0); }};
}
inline Isotropic fung(Coeff c, Coeff b) {
  return {[=](const Series &i1, const Series &) { return i1.sub(3.0).mul(b.on(i1)).exp().sub(1.0).mul(c.on(i1)).mul(0.5); },
          [=](const Series &i1, const Series &) { return i1.sub(3.0).mul(b.on(i1)).exp().mul(c.on(i1)).mul(b.on(i1)).mul(0.5); },
          [=](const Series &i1, const Series &) { return i1.mul(0.0); }};
}

inline Series first_invariant(const Series &l1, const Series &l2, const Series &l3) { return l1.square().add(l2.square()).add(l3.square()); }
inline Series second_invariant(const Series &l1, const Series &l2, const Series &l3) { return l1.pow(-2.0).add(l2.pow(-2.0)).add(l3.pow(-2.0)); }

inline Series uniaxial_stress(const Isotropic &m, const Series &lambda) {
  const Series l2 = lambda.pow(-0.5);
  const Series i1 = first_invariant(lambda, l2, l2), i2 = second_invariant(lambda, l2, l2);
  return lambda.square().sub(lambda.reciprocal()).mul(2.0).mul(m.w1(i1, i2).add(m.w2(i1, i2).div(lambda)));
}
inline Series equibiaxial_stress(const Isotropic &m, const Series &lambda) {
  const Series l3 = lambda.pow(-2.0);
  const Series i1 = first_invariant(lambda, lambda, l3), i2 = second_invariant(lambda, lambda, l3);
  return lambda.square().sub(lambda.pow(-4.0)).mul(2.0).mul(m.w1(i1, i2).add(m.w2(i1, i2).mul(lambda.square())));
}
inline Series planar_stress_difference(const Isotropic &m, const Series &lambda) { // sigma_1 - sigma_3 in planar tension (l2 = 1)
  const Series l3 = lambda.reciprocal();
  const Series i1 = first_invariant(lambda, lambda.mul(0.0).add(1.0), l3), i2 = second_invariant(lambda, lambda.mul(0.0).add(1.0), l3);
  return lambda.square().sub(lambda.pow(-2.0)).mul(2.0).mul(m.w1(i1, i2).add(m.w2(i1, i2)));
}
struct Shear final {
  Series shear_stress = pluss::series::undefined();      // sigma_12
  Series normal_difference = pluss::series::undefined(); // sigma_11 - sigma_22
};
inline Shear simple_shear(const Isotropic &m, const Series &gamma) {
  const Series i = gamma.square().add(3.0);
  return {gamma.mul(2.0).mul(m.w1(i, i).add(m.w2(i, i))), gamma.square().mul(2.0).mul(m.w1(i, i).add(m.w2(i, i)))};
}
// small-strain shear modulus mu0 = 2 (W1 + W2) at the undeformed state, evaluated on any Series lane shape (`like`)
inline Series initial_shear_modulus(const Isotropic &m, const Series &like) {
  const Series three = like.mul(0.0).add(3.0);
  return m.w1(three, three).add(m.w2(three, three)).mul(2.0);
}

// ---- Ogden (principal stretches; mu, alpha are [P] term lanes on the LAST axis, stretches [...]) -------------------------------------------
inline Series ogden_uniaxial(const Series &lambda, const Series &mu, const Series &alpha) {
  const Series l = lambda.unsqueeze(-1);
  return mu.mul(l.pow(alpha).sub(l.pow(alpha.mul(-0.5)))).sum(-1, false);
}
inline Series ogden_equibiaxial(const Series &lambda, const Series &mu, const Series &alpha) {
  const Series l = lambda.unsqueeze(-1);
  return mu.mul(l.pow(alpha).sub(l.pow(alpha.mul(-2.0)))).sum(-1, false);
}
inline Series ogden_planar_difference(const Series &lambda, const Series &mu, const Series &alpha) {
  const Series l = lambda.unsqueeze(-1);
  return mu.mul(l.pow(alpha).sub(l.pow(alpha.neg()))).sum(-1, false);
}
inline Series ogden_shear_modulus(const Series &mu, const Series &alpha) { return mu.mul(alpha).sum(-1, false).mul(0.5); }

// ---- fibre-reinforced artery (HGO / GOH) --------------------------------------------------------------------------------------------------
struct Hgo final {
  Coeff c10{1.0};     // neo-Hookean ground matrix, mu = 2 c10
  Coeff k1{1.0};      // fibre stiffness
  Coeff k2{1.0};      // fibre exponential rate
  Coeff kappa{0.0};   // dispersion, 0 <= kappa <= 1/3
  Coeff gamma{0.7854}; // fibre angle from the circumferential direction [rad]
};
struct ArteryStress final {
  Series theta_minus_radial = pluss::series::undefined(); // sigma_tt - sigma_rr
  Series axial_minus_radial = pluss::series::undefined(); // sigma_zz - sigma_rr
};
inline ArteryStress artery_stresses(const Hgo &m, const Series &lambda_theta, const Series &lambda_z) {
  const Series lr = lambda_theta.mul(lambda_z).reciprocal();
  const Series i1 = first_invariant(lambda_theta, lambda_z, lr);
  const Series g = m.gamma.on(lambda_theta), kappa = m.kappa.on(lambda_theta), k1 = m.k1.on(lambda_theta), k2 = m.k2.on(lambda_theta);
  const Series c2 = g.cos().square(), s2 = g.sin().square();
  const Series i4 = lambda_theta.square().mul(c2).add(lambda_z.square().mul(s2));
  const Series e = kappa.mul(i1.sub(3.0)).add(kappa.mul(-3.0).add(1.0).mul(i4.sub(1.0)));
  const Series ep = e.add(e.abs()).mul(0.5); // Macaulay bracket
  const Series dw_de = k1.mul(ep).mul(k2.mul(ep.square()).exp()); // dW_fibre/dE per family
  const Series w_i1 = m.c10.on(lambda_theta).add(dw_de.mul(kappa).mul(2.0)); // two families
  const Series w_i4 = dw_de.mul(kappa.mul(-3.0).add(1.0));                    // per family
  // l_i dW/dl_i with l_r independent: dI1/dl_i = 2 l_i, dI4/dl_t = 2 l_t cos^2 g, dI4/dl_z = 2 l_z sin^2 g, dI4/dl_r = 0
  const Series t = lambda_theta.square().mul(w_i1.mul(2.0).add(w_i4.mul(2.0).mul(c2).mul(2.0)));
  const Series z = lambda_z.square().mul(w_i1.mul(2.0).add(w_i4.mul(2.0).mul(s2).mul(2.0)));
  const Series r = lr.square().mul(w_i1.mul(2.0));
  return {t.sub(r), z.sub(r)};
}

} // namespace pluss::subsp::natsci::biology::biomechanics::tissue
