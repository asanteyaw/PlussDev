#pragma once

// Bead-spring polymer dynamics in dilute solution and melts: Rouse and Zimm models, batched over lanes of model parameters.
//
//  * Rouse (Rouse, J. Chem. Phys. 21 (1953) 1272): N beads joined by harmonic springs of constant k_s = 3 kT/b^2 with friction zeta per
//    bead, free ends. The connectivity matrix A (A_nn = 2, A_{n,n+-1} = -1, A_11 = A_NN = 1) has eigenvalues a_p = 4 sin^2(p pi / 2N),
//    p = 0 .. N-1 (p = 0 is the free translation), and the end-to-end relaxation times are tau_p = zeta b^2 / (3 kT a_p)
//    -> zeta N^2 b^2 / (3 pi^2 kT p^2) for p << N (Doi & Edwards, The Theory of Polymer Dynamics, 1986, ch. 4). Centre-of-mass
//    diffusion D = kT / (N zeta); stress relaxation modulus G(t) = c kT sum_{p>=1} exp(-2 t / tau_p) (c chains per volume) and the
//    zero-shear viscosity eta = c kT sum tau_p / 2 -> c zeta N^2 b^2 / 36.
//  * Zimm (Zimm, J. Chem. Phys. 24 (1956) 269): the same chain with the preaveraged Oseen hydrodynamic interaction
//    H_nm = delta_nm + sqrt(2) h* / sqrt(|n - m|), h* = zeta / (eta_s b sqrt(12 pi^3)); the relaxation times follow from the eigenvalues of
//    H A (computed exactly as the spectrum of the symmetric matrix L^T A L with H = L L^T, the two being similar), tau_p = zeta b^2 /
//    (3 kT lambda_p), and D = kT / (N^2 zeta) sum_{nm} H_nm. For large N: tau_p ~ p^{-3/2}, D ~ N^{-1/2}. h* = 0 recovers Rouse.

#include "pluss/subsp/math/linalg.h"
#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>
#include <numbers>
#include <vector>

namespace pluss::subsp::natsci::chemistry::polymer_dynamics {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace linalg = ::pluss::subsp::math::linalg;
using dynamics::Coeff;

// Rouse connectivity matrix [N, N]
inline Series rouse_matrix(std::int64_t n_beads, pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  dynamics::require(n_beads >= 2, "rouse: need at least two beads");
  std::vector<double> a(static_cast<std::size_t>(n_beads * n_beads), 0.0);
  for (std::int64_t i = 0; i < n_beads; ++i) {
    a[static_cast<std::size_t>(i * n_beads + i)] = (i == 0 || i == n_beads - 1) ? 1.0 : 2.0;
    if (i + 1 < n_beads) {
      a[static_cast<std::size_t>(i * n_beads + i + 1)] = -1.0;
      a[static_cast<std::size_t>((i + 1) * n_beads + i)] = -1.0;
    }
  }
  return pluss::make_series(a, {n_beads, n_beads}).to(dtype);
}

// eigenvalues a_p = 4 sin^2(p pi / 2N), p = 0 .. N-1, [N]
inline Series rouse_eigenvalues(std::int64_t n_beads, pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  std::vector<double> a(static_cast<std::size_t>(n_beads));
  for (std::int64_t p = 0; p < n_beads; ++p) {
    const double s = std::sin(static_cast<double>(p) * std::numbers::pi_v<double> / (2.0 * static_cast<double>(n_beads)));
    a[static_cast<std::size_t>(p)] = 4.0 * s * s;
  }
  return pluss::make_series(a, {n_beads}).to(dtype);
}

// relaxation times of the modes p = 1 .. N-1 [..., N-1] for friction zeta, bond length b and thermal energy kT (scalars or lane Series)
inline Series rouse_relaxation_times(std::int64_t n_beads, const Series &zeta, const Coeff &b, const Coeff &kt) {
  const Series modes = rouse_eigenvalues(n_beads, zeta.scalar_type()).slice(-1, 1, n_beads, 1);
  return zeta.unsqueeze(-1).mul(b.on(zeta).square().unsqueeze(-1)).div(kt.on(zeta).unsqueeze(-1).mul(3.0)).div(modes);
}
inline Series rouse_diffusion(std::int64_t n_beads, const Series &zeta, const Coeff &kt) {
  return kt.on(zeta).div(zeta.mul(static_cast<double>(n_beads)));
}
// G(t) = c kT sum_p exp(-2 t / tau_p); t [T] (or [..., T]) against relaxation times tau [..., P]: result [..., T]
inline Series rouse_modulus(const Series &t, const Series &tau, const Coeff &chains_per_volume, const Coeff &kt) {
  const Series decay = t.unsqueeze(-1).mul(-2.0).div(tau.unsqueeze(-2)).exp().sum(-1, false); // sum over modes
  return decay.mul(chains_per_volume.on(decay)).mul(kt.on(decay));
}
inline Series rouse_viscosity(const Series &tau, const Coeff &chains_per_volume, const Coeff &kt) {
  return tau.sum(-1, false).mul(0.5).mul(chains_per_volume.on(tau.select(-1, 0))).mul(kt.on(tau.select(-1, 0)));
}

// ---- Zimm ------------------------------------------------------------------------------------------------
// preaveraged Oseen hydrodynamic-interaction matrix H [N, N] for the parameter h*
inline Series zimm_hydrodynamic_matrix(std::int64_t n_beads, double h_star) {
  std::vector<double> h(static_cast<std::size_t>(n_beads * n_beads), 0.0);
  for (std::int64_t i = 0; i < n_beads; ++i)
    for (std::int64_t j = 0; j < n_beads; ++j)
      h[static_cast<std::size_t>(i * n_beads + j)] = i == j ? 1.0 : std::sqrt(2.0) * h_star / std::sqrt(static_cast<double>(std::abs(i - j)));
  return pluss::make_series(h, {n_beads, n_beads}).to(pluss::ScalarType::Float64);
}
// eigenvalues lambda_p of H A (ascending), p = 0 .. N-1; lambda_0 = 0 (translation)
inline Series zimm_eigenvalues(std::int64_t n_beads, double h_star) {
  const Series a = rouse_matrix(n_beads);
  const Series h = zimm_hydrodynamic_matrix(n_beads, h_star);
  const Series l = linalg::cholesky(h, false); // H = L L^T, L lower triangular
  const Series s = l.transpose(0, 1).matmul(a).matmul(l);
  return linalg::eigh(s.add(s.transpose(0, 1)).mul(0.5)).eigenvalues;
}
// relaxation times of the modes p = 1 .. N-1: tau_p = zeta b^2 / (3 kT lambda_p)
inline Series zimm_relaxation_times(std::int64_t n_beads, double h_star, double zeta, double b, double kt) {
  const Series lambda = zimm_eigenvalues(n_beads, h_star).slice(-1, 1, n_beads, 1);
  return lambda.reciprocal().mul(zeta * b * b / (3.0 * kt));
}
// centre-of-mass diffusion coefficient D = kT / (N^2 zeta) sum_{nm} H_nm
inline double zimm_diffusion(std::int64_t n_beads, double h_star, double zeta, double kt) {
  const double total = zimm_hydrodynamic_matrix(n_beads, h_star).sum().item<double>();
  return kt / (static_cast<double>(n_beads) * static_cast<double>(n_beads) * zeta) * total;
}

} // namespace pluss::subsp::natsci::chemistry::polymer_dynamics
