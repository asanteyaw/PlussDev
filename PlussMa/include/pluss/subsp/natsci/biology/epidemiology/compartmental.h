#pragma once

// Next-generation basic reproduction number for a general compartmental
// model, exactly as defined in
//   P. van den Driessche & J. Watmough, "Reproduction numbers and
//   sub-threshold endemic equilibria for compartmental models of disease
//   transmission", Math. Biosci. 180 (2002) 29-48:
//     x' = F(x) - V(x),  V = V^- - V^+,  F: rate of NEW infections,
//     F = dF/dx|DFE, V = dV/dx|DFE restricted to the m infected
//     compartments, R0 = rho(F V^-1)   (their eq. (4)),
//     DFE stable iff R0 < 1 (their Theorem 2).
// The split into new infections vs. transfers is epidemiological, not
// structural; the caller supplies it. Assumption checks (A1)-(A5 subset):
// F >= 0, V a nonsingular M-matrix (off-diagonal <= 0).

#include "pluss/subsp/natsci/biology/dynamics.h"
#include "pluss/subsp/natsci/biology/population/structured.h"

namespace pluss::subsp::natsci::biology::epidemiology {

struct Split final {
  dynamics::RateFn infections; // F(x): new infections, length-n output
  dynamics::RateFn transfers;  // V(x) = V^-(x) - V^+(x), length-n output
  std::int64_t infected{0};    // m: number of infected compartments
  // Indices of the infected compartments in the state vector; empty means
  // the first m (the layout used by van den Driessche & Watmough).
  std::vector<std::int64_t> infected_index{};
};

// F V^-1 over the infected block, [..., m, m], at the disease-free state.
inline Series next_generation(const Split &split, const Series &dfe,
                              double step = 1e-6) {
  dynamics::require(split.infections && split.transfers && split.infected > 0,
                    "epidemiology: split needs F, V and m > 0");
  const std::int64_t m = split.infected;
  dynamics::require(m <= dfe.size(-1), "epidemiology: m exceeds compartments");
  const auto d = dynamics::Difference::ForwardSecondOrder;
  const Series jf = dynamics::jacobian(split.infections, dfe, step, d);
  const Series jv = dynamics::jacobian(split.transfers, dfe, step, d);
  std::vector<std::int64_t> index = split.infected_index;
  if (index.empty())
    for (std::int64_t i = 0; i < m; ++i)
      index.push_back(i);
  dynamics::require(static_cast<std::int64_t>(index.size()) == m,
                    "epidemiology: infected_index must list m compartments");
  // one gather per axis instead of an m x m select/unite loop
  const Series idx = pluss::make_series(index, {m}).to(pluss::ScalarType::Int64).to(dfe.platform());
  const auto block = [&](const Series &jac) { return jac.index_select(-2, idx).index_select(-1, idx); };
  const Series f = block(jf);
  const Series v = block(jv);
  // M-matrix sign pattern: off-diagonal entries of V are <= 0.
  const Config cfg =
      Config{}.with_dtype(dfe.scalar_type()).with_platform(dfe.platform());
  const Series off = pluss::series::eye(m, cfg).neg().add(1.0);
  const double scale = std::max(1.0, v.abs().max().item<double>());
  dynamics::require(v.mul(off).max().item<double>() <= 1e-6 * scale,
                    "epidemiology: V must have non-positive off-diagonal "
                    "entries (is the F/V split epidemiologically consistent?)");
  dynamics::require(f.min().item<double>() >= -1e-6 * scale,
                    "epidemiology: F must be non-negative at the DFE");
  return f.matmul(pluss::subsp::linearalg::inverse(v));
}

// R0 = spectral radius of the next-generation matrix (batched over lanes).
inline Series reproduction_number(const Split &split, const Series &dfe,
                                  double step = 1e-6) {
  const Series ngm = next_generation(split, dfe, step).clamp(0.0, 1e300);
  return population::structured::dominant(ngm).lambda;
}

} // namespace pluss::subsp::natsci::biology::epidemiology
