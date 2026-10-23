#pragma once

// Thermodynamic constraints on metabolic pathways and networks, on the sparse LP / MILP stack (math/lp).
//
//  * Max-min driving force (Noor, Bar-Even, Flamholz, Reznik, Liebermeister & Milo, "Pathway thermodynamics highlights kinetic obstacles in
//    central metabolism", PLoS Comput. Biol. 10 (2014) e1003483, read from the open-access article): for a pathway with stoichiometric matrix S
//    (compounds x reactions, reactions oriented along the net flux), standard transformed reaction energies G0 and log concentrations x,
//    the driving force of reaction j is -(G0_j + RT (S^T x)_j) and
//        maximise B  subject to  -(G0 + RT S^T x) >= B,   ln Cmin <= x <= ln Cmax   (+ fixed ratios such as ATP/ADP),
//    a linear program in (x, B). The pathway is thermodynamically feasible iff the optimal B (the MDF) is positive; the dual variables of the
//    driving-force rows are the reaction shadow prices that identify the thermodynamic bottlenecks.
//  * Thermodynamics-based flux balance (Henry, Broadbelt & Hatzimanikatis, Biophys. J. 92 (2007) 1792): fluxes split into forward and reverse
//    parts v+, v- with a binary z_j per reaction; z_j = 1 allows forward flux only if dG_j = G0_j + RT (S^T x)_j < 0 (big-M), z_j = 0
//    allows reverse flux only if dG_j > 0; maximise the usual objective over (v, x, z) as a mixed-integer program.
//  Energies in kJ/mol, concentrations in M (the standard state is 1 M).

#include "pluss/subsp/math/lp/mip.h"
#include "pluss/subsp/math/lp/solve.h"
#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <vector>

namespace pluss::subsp::natsci::biology::metabolism::pathway_thermodynamics {

namespace dynamics = ::pluss::subsp::natsci::biology::dynamics;
namespace lp = ::pluss::subsp::math::lp;

inline double gas_constant_kj() { return 8.314462618e-3; } // kJ/(mol K)

// concentration ratio constraint [num]/[den] = ratio (e.g. ATP/ADP = 10)
struct Ratio final {
  std::int64_t numerator{0}, denominator{0};
  double ratio{1.0};
};

struct Pathway final {
  Series stoichiometry = pluss::series::undefined(); // [C compounds, R reactions]
  Series dg0 = pluss::series::undefined();           // [R] transformed standard reaction energy (kJ/mol)
  std::vector<double> c_min, c_max;                  // [C] concentration bounds (M)
  std::vector<Ratio> ratios;
  double temperature{298.15};
};

struct MdfResult final {
  lp::SolveStatus status{lp::SolveStatus::Infeasible};
  double mdf{0.0};                                 // kJ/mol; positive = feasible
  std::vector<double> ln_concentration;            // [C]
  std::vector<double> driving_force;               // [R] kJ/mol at the optimum
  std::vector<double> shadow_price;                // [R] reaction shadow prices (>= 0; > 0 for bottlenecks)
};

namespace detail {
inline std::vector<double> host(const Series &s) { return s.to(pluss::ScalarType::Float64).contiguous().to_vec<double>(); }
} // namespace detail

inline MdfResult max_min_driving_force(const Pathway &p) {
  const std::int64_t c = p.stoichiometry.size(0), r = p.stoichiometry.size(1);
  dynamics::require(static_cast<std::int64_t>(p.c_min.size()) == c && static_cast<std::int64_t>(p.c_max.size()) == c, "mdf: one concentration bound per compound");
  const auto s = detail::host(p.stoichiometry), g0 = detail::host(p.dg0);
  const double rt = gas_constant_kj() * p.temperature;
  // variables: x_0..x_{C-1}, B ; rows: R driving-force rows (RT S^T x + B <= -G0), then the ratio equalities
  lp::Model m;
  m.cols = static_cast<std::int32_t>(c + 1);
  m.rows = static_cast<std::int32_t>(r + static_cast<std::int64_t>(p.ratios.size()));
  m.cost.assign(static_cast<std::size_t>(m.cols), 0.0);
  m.cost.back() = -1.0; // minimise -B
  for (std::int64_t i = 0; i < c; ++i) {
    m.col_lo.push_back(std::log(p.c_min[static_cast<std::size_t>(i)]));
    m.col_hi.push_back(std::log(p.c_max[static_cast<std::size_t>(i)]));
  }
  m.col_lo.push_back(-1e4);
  m.col_hi.push_back(1e4);
  std::vector<std::int32_t> tr, tc;
  std::vector<double> tv;
  for (std::int64_t j = 0; j < r; ++j) {
    for (std::int64_t i = 0; i < c; ++i) {
      const double v = s[static_cast<std::size_t>(i * r + j)];
      if (v != 0.0) {
        tr.push_back(static_cast<std::int32_t>(j));
        tc.push_back(static_cast<std::int32_t>(i));
        tv.push_back(rt * v);
      }
    }
    tr.push_back(static_cast<std::int32_t>(j));
    tc.push_back(static_cast<std::int32_t>(c));
    tv.push_back(1.0);
    m.row_lo.push_back(-lp::kInf);
    m.row_hi.push_back(-g0[static_cast<std::size_t>(j)]);
  }
  for (std::size_t k = 0; k < p.ratios.size(); ++k) {
    const auto row = static_cast<std::int32_t>(r + static_cast<std::int64_t>(k));
    tr.push_back(row); tc.push_back(static_cast<std::int32_t>(p.ratios[k].numerator)); tv.push_back(1.0);
    tr.push_back(row); tc.push_back(static_cast<std::int32_t>(p.ratios[k].denominator)); tv.push_back(-1.0);
    m.row_lo.push_back(std::log(p.ratios[k].ratio));
    m.row_hi.push_back(std::log(p.ratios[k].ratio));
  }
  m.a = lp::SparseMatrix::from_triplets(m.rows, m.cols, tr, tc, tv);
  const auto sol = lp::solve(m);
  MdfResult out;
  out.status = sol.status;
  if (sol.status != lp::SolveStatus::Optimal)
    return out;
  out.mdf = sol.x.back();
  out.ln_concentration.assign(sol.x.begin(), sol.x.end() - 1);
  for (std::int64_t j = 0; j < r; ++j) {
    double act = 0.0;
    for (std::int64_t i = 0; i < c; ++i)
      act += s[static_cast<std::size_t>(i * r + j)] * out.ln_concentration[static_cast<std::size_t>(i)];
    out.driving_force.push_back(-(g0[static_cast<std::size_t>(j)] + rt * act));
    out.shadow_price.push_back(std::abs(sol.row_dual[static_cast<std::size_t>(j)]));
  }
  return out;
}

// ---- thermodynamics-based flux balance ----------------------------------------------------------------------------------------
struct TfaNetwork final {
  Series stoichiometry = pluss::series::undefined(); // [C, R]
  Series dg0 = pluss::series::undefined();           // [R] kJ/mol (ignored where constrained[j] == 0)
  std::vector<double> lower, upper;                  // [R] net flux bounds (v_lower may be negative)
  std::vector<char> constrained;                     // [R] 1 when the reaction obeys the second law (0 for exchanges / biomass)
  std::vector<double> c_min, c_max;                  // [C]
  std::vector<double> objective;                     // [R] maximise sum c_j v_j
  double temperature{298.15};
  double epsilon{1e-6};                              // kJ/mol margin of the strict inequalities
  double big_m_flux{1e3};
};
struct TfaResult final {
  lp::SolveStatus status{lp::SolveStatus::Infeasible};
  double objective{0.0};
  std::vector<double> flux;           // net v = v+ - v-
  std::vector<double> ln_concentration;
  std::vector<double> delta_g;        // kJ/mol (constrained reactions)
  std::vector<char> forward;          // z_j
};

inline TfaResult thermodynamic_flux_balance(const TfaNetwork &net, const lp::MipOptions &opt = {}) {
  const std::int64_t c = net.stoichiometry.size(0), r = net.stoichiometry.size(1);
  const auto s = detail::host(net.stoichiometry), g0 = detail::host(net.dg0);
  const double rt = gas_constant_kj() * net.temperature;
  // value ranges of dG_j implied by the concentration bounds give the big-M constant of each reaction
  std::vector<double> big_k(static_cast<std::size_t>(r), 0.0);
  for (std::int64_t j = 0; j < r; ++j) {
    double lo = g0[static_cast<std::size_t>(j)], hi = lo;
    for (std::int64_t i = 0; i < c; ++i) {
      const double v = s[static_cast<std::size_t>(i * r + j)] * rt;
      if (v > 0) { lo += v * std::log(net.c_min[static_cast<std::size_t>(i)]); hi += v * std::log(net.c_max[static_cast<std::size_t>(i)]); }
      else if (v < 0) { lo += v * std::log(net.c_max[static_cast<std::size_t>(i)]); hi += v * std::log(net.c_min[static_cast<std::size_t>(i)]); }
    }
    big_k[static_cast<std::size_t>(j)] = std::max(std::abs(lo), std::abs(hi)) + 1.0;
  }
  // variable layout: vf[0..R), vr[R..2R), x[2R..2R+C), z[2R+C..3R+C)
  const std::int64_t ivf = 0, ivr = r, ix = 2 * r, iz = 2 * r + c;
  lp::Model m;
  m.cols = static_cast<std::int32_t>(3 * r + c);
  m.cost.assign(static_cast<std::size_t>(m.cols), 0.0);
  m.col_lo.assign(static_cast<std::size_t>(m.cols), 0.0);
  m.col_hi.assign(static_cast<std::size_t>(m.cols), 0.0);
  m.integer.assign(static_cast<std::size_t>(m.cols), 0);
  for (std::int64_t j = 0; j < r; ++j) {
    const double lo = net.lower[static_cast<std::size_t>(j)], hi = net.upper[static_cast<std::size_t>(j)];
    m.col_hi[static_cast<std::size_t>(ivf + j)] = std::max(0.0, hi);
    m.col_hi[static_cast<std::size_t>(ivr + j)] = std::max(0.0, -lo);
    m.cost[static_cast<std::size_t>(ivf + j)] = -net.objective[static_cast<std::size_t>(j)];
    m.cost[static_cast<std::size_t>(ivr + j)] = net.objective[static_cast<std::size_t>(j)];
    m.col_lo[static_cast<std::size_t>(iz + j)] = 0.0;
    m.col_hi[static_cast<std::size_t>(iz + j)] = 1.0;
    m.integer[static_cast<std::size_t>(iz + j)] = 1;
  }
  for (std::int64_t i = 0; i < c; ++i) {
    m.col_lo[static_cast<std::size_t>(ix + i)] = std::log(net.c_min[static_cast<std::size_t>(i)]);
    m.col_hi[static_cast<std::size_t>(ix + i)] = std::log(net.c_max[static_cast<std::size_t>(i)]);
  }
  std::vector<std::int32_t> tr, tc;
  std::vector<double> tv;
  std::int32_t row = 0;
  const auto push = [&](std::int32_t col, double v) {
    tr.push_back(row);
    tc.push_back(col);
    tv.push_back(v);
  };
  for (std::int64_t i = 0; i < c; ++i) { // steady state: S (v+ - v-) = 0
    for (std::int64_t j = 0; j < r; ++j) {
      const double v = s[static_cast<std::size_t>(i * r + j)];
      if (v != 0.0) {
        push(static_cast<std::int32_t>(ivf + j), v);
        push(static_cast<std::int32_t>(ivr + j), -v);
      }
    }
    m.row_lo.push_back(0.0);
    m.row_hi.push_back(0.0);
    ++row;
  }
  for (std::int64_t j = 0; j < r; ++j) {
    if (!net.constrained[static_cast<std::size_t>(j)])
      continue;
    const double vm = net.big_m_flux, k = big_k[static_cast<std::size_t>(j)] + net.epsilon;
    // v+ <= M z ;  v- <= M (1 - z)
    push(static_cast<std::int32_t>(ivf + j), 1.0); push(static_cast<std::int32_t>(iz + j), -vm); m.row_lo.push_back(-lp::kInf); m.row_hi.push_back(0.0); ++row;
    push(static_cast<std::int32_t>(ivr + j), 1.0); push(static_cast<std::int32_t>(iz + j), vm); m.row_lo.push_back(-lp::kInf); m.row_hi.push_back(vm); ++row;
    // forward allowed => G0 + RT S^T x <= -eps :  RT S^T x + K z <= -G0 - eps + K
    for (std::int64_t i = 0; i < c; ++i)
      if (s[static_cast<std::size_t>(i * r + j)] != 0.0)
        push(static_cast<std::int32_t>(ix + i), rt * s[static_cast<std::size_t>(i * r + j)]);
    push(static_cast<std::int32_t>(iz + j), k);
    m.row_lo.push_back(-lp::kInf); m.row_hi.push_back(-g0[static_cast<std::size_t>(j)] - net.epsilon + k); ++row;
    // reverse allowed (z = 0) => G0 + RT S^T x >= eps :  RT S^T x - K z >= -G0 + eps - K ... written as RT S^T x + K z' with z' = z: dG >= eps - K z
    for (std::int64_t i = 0; i < c; ++i)
      if (s[static_cast<std::size_t>(i * r + j)] != 0.0)
        push(static_cast<std::int32_t>(ix + i), rt * s[static_cast<std::size_t>(i * r + j)]);
    push(static_cast<std::int32_t>(iz + j), k);
    m.row_lo.push_back(-g0[static_cast<std::size_t>(j)] + net.epsilon); m.row_hi.push_back(lp::kInf); ++row;
  }
  m.rows = row;
  m.a = lp::SparseMatrix::from_triplets(m.rows, m.cols, tr, tc, tv);
  const auto sol = lp::solve_mip(m, opt);
  TfaResult out;
  out.status = sol.status;
  if (sol.status != lp::SolveStatus::Optimal)
    return out;
  out.objective = -sol.objective;
  for (std::int64_t j = 0; j < r; ++j) {
    out.flux.push_back(sol.x[static_cast<std::size_t>(ivf + j)] - sol.x[static_cast<std::size_t>(ivr + j)]);
    out.forward.push_back(sol.x[static_cast<std::size_t>(iz + j)] > 0.5 ? 1 : 0);
  }
  for (std::int64_t i = 0; i < c; ++i)
    out.ln_concentration.push_back(sol.x[static_cast<std::size_t>(ix + i)]);
  for (std::int64_t j = 0; j < r; ++j) {
    double act = 0.0;
    for (std::int64_t i = 0; i < c; ++i)
      act += s[static_cast<std::size_t>(i * r + j)] * out.ln_concentration[static_cast<std::size_t>(i)];
    out.delta_g.push_back(g0[static_cast<std::size_t>(j)] + rt * act);
  }
  return out;
}

} // namespace pluss::subsp::natsci::biology::metabolism::pathway_thermodynamics
