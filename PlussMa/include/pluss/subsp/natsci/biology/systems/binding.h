#pragma once

// Ligand binding equilibria, batched over lanes (occupancy curves via kinetics::hill_activation with Vmax = 1).
//   receptor_ligand: exact 1:1 binding with ligand depletion (cancellation-free quadratic root)
//     RL = 2 R_t L_t / (R_t + L_t + K_d + sqrt((R_t + L_t + K_d)^2 - 4 R_t L_t))
//   mwc: Monod-Wyman-Changeux concerted allostery (n identical sites, R/T conformers, L = [T]/[R] without
//     ligand, c = K_R/K_T, alpha = [S]/K_R):
//       Y = [alpha (1+alpha)^(n-1) + L c alpha (1+c alpha)^(n-1)] / [(1+alpha)^n + L (1+c alpha)^n]
//   competitive: one receptor, several ligands with depletion, via common::speciation (batched Newton)
//   adair: n sites with macroscopic (cumulative) constants beta_k = [RL_k]/([R][L]^k), mean occupancy
//     Y = sum_k k beta_k L^k / (n (1 + sum_k beta_k L^k)); with ligand depletion through speciation.

#include "pluss/subsp/natsci/biology/dynamics.h"
#include "pluss/subsp/natsci/biology/systems/kinetics.h"
#include "pluss/subsp/natsci/common/speciation.h"

namespace pluss::subsp::natsci::biology::systems::binding {

namespace speciation = ::pluss::subsp::natsci::common::speciation;
using dynamics::Coeff;

inline Series receptor_ligand(const Series &receptor_total, const Series &ligand_total, const Coeff &kd) {
  dynamics::require_positive(kd, receptor_total, "receptor_ligand: Kd");
  dynamics::require_nonnegative(receptor_total, "receptor_ligand: receptor total");
  dynamics::require_nonnegative(ligand_total, "receptor_ligand: ligand total");
  const Series s = receptor_total.add(ligand_total).add(kd.on(receptor_total));
  const Series disc = s.square().sub(receptor_total.mul(ligand_total).mul(4.0)).clamp_min(0.0).sqrt();
  return receptor_total.mul(ligand_total).mul(2.0).div(s.add(disc));
}

// Fractional saturation Y of the MWC model for site count n (integer >= 1).
inline Series mwc(const Series &ligand, const Coeff &k_r, const Coeff &c, const Coeff &l0, int n) {
  dynamics::require(n >= 1, "mwc: need at least one site");
  dynamics::require_positive(k_r, ligand, "mwc: K_R");
  dynamics::require_positive(c, ligand, "mwc: c");
  dynamics::require_nonnegative(l0, ligand, "mwc: L");
  dynamics::require_nonnegative(ligand, "mwc: ligand");
  const Series alpha = ligand.div(k_r.on(ligand));
  const Series ca = alpha.mul(c.on(ligand)), l = l0.on(ligand);
  const double nn = static_cast<double>(n);
  const Series r = alpha.add(1.0), t = ca.add(1.0);
  const Series num = alpha.mul(r.pow(nn - 1.0)).add(l.mul(ca).mul(t.pow(nn - 1.0)));
  return num.div(r.pow(nn).add(l.mul(t.pow(nn))));
}

struct Competitive final {
  Series occupancy = pluss::series::undefined();    // [..., L] bound fraction of the receptor per ligand
  Series free_receptor = pluss::series::undefined(); // [...]
  Series free_ligand = pluss::series::undefined();   // [..., L]
};

// receptor_total [...], ligand_total [..., L], kd [..., L] (or [L]).
inline Competitive competitive(const Series &receptor_total, const Series &ligand_total, const Series &kd) {
  dynamics::require_positive(kd, "competitive: Kd");
  const std::int64_t nl = ligand_total.size(-1);
  const Config cfg = Config{}.with_dtype(ligand_total.scalar_type()).with_platform(ligand_total.platform());
  // components [R, L_1..L_L], complexes R.L_j  (stoichiometry [1+L, L])
  const Series stoich = pluss::series::zeros({1, nl}, cfg).add(1.0).unite(std::vector<Series>{pluss::series::eye(nl, cfg)}, 0);
  const Series totals = receptor_total.unsqueeze(-1).unite(std::vector<Series>{ligand_total}, -1);
  const auto sol = speciation::solve(stoich, kd.log().neg().add(ligand_total.mul(0.0)), totals);
  return {sol.complexes.div(receptor_total.unsqueeze(-1)), sol.free.select(-1, 0), sol.free.slice(-1, 1, nl + 1, 1)};
}

// Mean occupancy per site of an n-site receptor with cumulative constants beta_k (ln_beta [..., n]).
inline Series adair(const Series &receptor_total, const Series &ligand_total, const Series &ln_beta) {
  const std::int64_t n = ln_beta.size(-1);
  const Config cfg = Config{}.with_dtype(receptor_total.scalar_type()).with_platform(receptor_total.platform());
  // components [R, L]; complexes RL_k with N = [[1,...],[1,2,...,n]]
  const Series row_l = pluss::series::arange(1.0, static_cast<double>(n) + 1.0, 1.0, cfg).unsqueeze(0); // [1, n]
  const Series stoich = row_l.mul(0.0).add(1.0).unite(std::vector<Series>{row_l}, 0);
  const Series totals = receptor_total.unsqueeze(-1).unite(std::vector<Series>{ligand_total.unsqueeze(-1)}, -1);
  const auto sol = speciation::solve(stoich, ln_beta, totals);
  return sol.complexes.mul(row_l.squeeze(0)).sum(-1, false).div(receptor_total.mul(static_cast<double>(n)));
}

} // namespace pluss::subsp::natsci::biology::systems::binding
