#pragma once

// Elementary flux modes (EFMs) and flux-space sampling of a stoichiometric network  S v = 0,  l <= v <= u.
//
//  * elementary_modes: the nullspace / double-description algorithm (Schuster & Hilgetag, J. Biol. Syst. 2 (1994) 165; Urbanczik & Wagner,
//    Bioinformatics 21 (2005) 4021; Terzer & Stelling, Bioinformatics 24 (2008) 2229 -- combinatorial adjacency test on the processed rows --
//    none of which was obtainable, the implementation is verified against brute-force enumeration of minimal supports). Reversible reactions
//    are split into forward/backward irreversible columns, the 2-cycles this creates are dropped and the rest mapped back, so a mode that uses a
//    reversible reaction appears once with its natural sign. The enumeration is inherently combinatorial (pair generation and subset tests on
//    supports), so it runs on host vectors; the result is returned as a Series [modes, reactions] with max |v| = 1.
//  * sample_fluxes: hit-and-run (Smith 1984) over the polytope, `chains` independent chains advancing together as lanes of Series ops:
//    random direction in the nullspace of [S; fixed-flux rows], exact feasible interval by vectorised ratio tests, uniform step; the start is the
//    Chebyshev centre (LP on the shared sparse/dense solver). Unbounded sides (infinities or sentinels such as 1e300) are truncated at +-bound_cap
//    (default 1000, the COBRA convention), reactions pinned by the network (blocked, implied equalities) are detected by flux variability and held
//    exactly, and a polytope that still has no interior raises instead of returning degenerate samples.

#include "pluss/subsp/linearalg/linearalg.h"
#include "pluss/subsp/natsci/biology/metabolism/fba.h"
#include "pluss/tils/aux/rng_seed.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace pluss::subsp::natsci::biology::metabolism {

// ---- elementary flux modes -----------------------------------------------------------------------------------------------------
namespace detail {
struct Column {
  std::vector<double> v;           // kernel vector over all (split) reactions
  std::vector<std::uint64_t> zero; // zero-set over processed rows (bit i set <=> v[row_i] == 0)
};
inline bool subset(const std::vector<std::uint64_t> &a, const std::vector<std::uint64_t> &b) { // a subset of b
  for (std::size_t i = 0; i < a.size(); ++i)
    if (a[i] & ~b[i])
      return false;
  return true;
}
} // namespace detail

inline Series elementary_modes(const Network &net, double tolerance = 1e-9, std::size_t max_modes = 2'000'000) {
  validate(net);
  const std::int64_t m = net.stoichiometry.size(0), r = net.stoichiometry.size(1);
  const auto s_host = net.stoichiometry.to(pluss::ScalarType::Float64).contiguous().to_vec<double>();
  const auto lo = net.lower.to(pluss::ScalarType::Float64).contiguous().to_vec<double>();
  const auto hi = net.upper.to(pluss::ScalarType::Float64).contiguous().to_vec<double>();
  dynamics::require(net.lower.dim() == 1, "elementary_modes: bounds must be [r]");
  // split: columns of the irreversible network and their origin (reaction, sign)
  std::vector<std::int64_t> origin;
  std::vector<int> sign;
  for (std::int64_t j = 0; j < r; ++j) {
    const bool fwd = hi[static_cast<std::size_t>(j)] > 0.0, bwd = lo[static_cast<std::size_t>(j)] < 0.0;
    if (fwd) { origin.push_back(j); sign.push_back(1); }
    if (bwd) { origin.push_back(j); sign.push_back(-1); }
  }
  const std::int64_t n = static_cast<std::int64_t>(origin.size());
  std::vector<std::vector<double>> sp(static_cast<std::size_t>(m), std::vector<double>(static_cast<std::size_t>(n)));
  for (std::int64_t i = 0; i < m; ++i)
    for (std::int64_t c = 0; c < n; ++c)
      sp[static_cast<std::size_t>(i)][static_cast<std::size_t>(c)] = sign[static_cast<std::size_t>(c)] * s_host[static_cast<std::size_t>(i * r + origin[static_cast<std::size_t>(c)])];
  const auto split_s = sp; // [m][n] split stoichiometry (before elimination), for the final elementarity (rank) test
  // Gauss-Jordan with column pivoting -> kernel basis with an identity block on the free columns
  std::vector<std::int64_t> perm(static_cast<std::size_t>(n));
  for (std::int64_t c = 0; c < n; ++c)
    perm[static_cast<std::size_t>(c)] = c;
  std::int64_t rank = 0;
  for (std::int64_t col = 0; col < n && rank < m; ++col) {
    std::int64_t piv = -1;
    double best = tolerance;
    for (std::int64_t i = rank; i < m; ++i)
      if (std::abs(sp[static_cast<std::size_t>(i)][static_cast<std::size_t>(col)]) > best) { best = std::abs(sp[static_cast<std::size_t>(i)][static_cast<std::size_t>(col)]); piv = i; }
    if (piv < 0)
      continue;
    std::swap(sp[static_cast<std::size_t>(piv)], sp[static_cast<std::size_t>(rank)]);
    const double d = sp[static_cast<std::size_t>(rank)][static_cast<std::size_t>(col)];
    for (auto &x : sp[static_cast<std::size_t>(rank)]) x /= d;
    for (std::int64_t i = 0; i < m; ++i) {
      if (i == rank) continue;
      const double f = sp[static_cast<std::size_t>(i)][static_cast<std::size_t>(col)];
      if (f == 0.0) continue;
      for (std::int64_t c = 0; c < n; ++c) sp[static_cast<std::size_t>(i)][static_cast<std::size_t>(c)] -= f * sp[static_cast<std::size_t>(rank)][static_cast<std::size_t>(c)];
    }
    // record the pivot column in position `rank` of the pivot list
    std::swap(perm[static_cast<std::size_t>(rank)], perm[static_cast<std::size_t>(col)]);
    for (std::int64_t i = 0; i < m; ++i) std::swap(sp[static_cast<std::size_t>(i)][static_cast<std::size_t>(rank)], sp[static_cast<std::size_t>(i)][static_cast<std::size_t>(col)]);
    ++rank;
  }
  const std::int64_t d = n - rank;
  std::vector<detail::Column> cols;
  if (d == 0)
    return pluss::make_series(std::vector<double>{}, {0, r}).to(pluss::ScalarType::Float64);
  // kernel vectors (in permuted coordinates): free coordinate k set to 1, pivot coordinate i = -RREF[i][rank + k]
  const std::int64_t words = (n + 63) / 64;
  // processing order: the first `d` rows (free coordinates) are identity -> already non-negative; the pivot rows are processed afterwards
  std::vector<std::int64_t> order; // rows of the permuted coordinate vector to process, in sequence
  for (std::int64_t i = 0; i < rank; ++i) order.push_back(i);
  cols.resize(static_cast<std::size_t>(d));
  for (std::int64_t k = 0; k < d; ++k) {
    auto &c = cols[static_cast<std::size_t>(k)];
    c.v.assign(static_cast<std::size_t>(n), 0.0);
    c.v[static_cast<std::size_t>(rank + k)] = 1.0;
    for (std::int64_t i = 0; i < rank; ++i) c.v[static_cast<std::size_t>(i)] = -sp[static_cast<std::size_t>(i)][static_cast<std::size_t>(rank + k)];
    c.zero.assign(static_cast<std::size_t>(words), 0);
  }
  // the identity rows are processed implicitly: record zero bits for them (bit index = permuted row index)
  const auto set_bits = [&](detail::Column &c, const std::vector<std::int64_t> &rows) {
    std::fill(c.zero.begin(), c.zero.end(), 0);
    for (std::int64_t row : rows)
      if (std::abs(c.v[static_cast<std::size_t>(row)]) <= tolerance) c.zero[static_cast<std::size_t>(row / 64)] |= (1ULL << (row % 64));
  };
  std::vector<std::int64_t> processed;
  for (std::int64_t k = 0; k < d; ++k) processed.push_back(rank + k);
  for (auto &c : cols) set_bits(c, processed);
  // process pivot rows one at a time, choosing the row with the fewest positive x negative pairs next
  std::vector<std::int64_t> pending = order;
  while (!pending.empty()) {
    std::size_t best_idx = 0;
    std::size_t best_pairs = std::numeric_limits<std::size_t>::max();
    for (std::size_t q = 0; q < pending.size(); ++q) {
      std::size_t pos = 0, neg = 0;
      for (const auto &c : cols) {
        const double x = c.v[static_cast<std::size_t>(pending[q])];
        if (x > tolerance) ++pos; else if (x < -tolerance) ++neg;
      }
      if (pos * neg < best_pairs) { best_pairs = pos * neg; best_idx = q; }
    }
    const std::int64_t row = pending[best_idx];
    pending.erase(pending.begin() + static_cast<std::ptrdiff_t>(best_idx));
    processed.push_back(row);
    std::vector<detail::Column> next;
    std::vector<std::size_t> pos, neg;
    for (std::size_t q = 0; q < cols.size(); ++q) {
      const double x = cols[q].v[static_cast<std::size_t>(row)];
      if (x > tolerance) pos.push_back(q); else if (x < -tolerance) neg.push_back(q); else { next.push_back(cols[q]); next.back().v[static_cast<std::size_t>(row)] = 0.0; }
    }
    // columns kept unchanged get the new row's zero bit; non-negative columns also stay
    for (auto &c : next) c.zero[static_cast<std::size_t>(row / 64)] |= (1ULL << (row % 64));
    for (std::size_t p : pos) { auto c = cols[p]; c.v[static_cast<std::size_t>(row)] = c.v[static_cast<std::size_t>(row)]; next.push_back(c); }
    const std::size_t kept = next.size();
    std::vector<detail::Column> fresh;
    for (std::size_t p : pos)
      for (std::size_t q : neg) {
        std::vector<std::uint64_t> common(static_cast<std::size_t>(words));
        for (std::int64_t w = 0; w < words; ++w) common[static_cast<std::size_t>(w)] = cols[p].zero[static_cast<std::size_t>(w)] & cols[q].zero[static_cast<std::size_t>(w)];
        common[static_cast<std::size_t>(row / 64)] |= (1ULL << (row % 64)); // the combination is zero on the new row
        bool adjacent = true;
        for (std::size_t o = 0; o < cols.size() && adjacent; ++o) {
          if (o == p || o == q) continue;
          std::vector<std::uint64_t> zo = cols[o].zero;
          if (std::abs(cols[o].v[static_cast<std::size_t>(row)]) <= tolerance) zo[static_cast<std::size_t>(row / 64)] |= (1ULL << (row % 64));
          if (detail::subset(common, zo)) adjacent = false;
        }
        if (!adjacent) continue;
        detail::Column c;
        const double a = cols[p].v[static_cast<std::size_t>(row)], b = -cols[q].v[static_cast<std::size_t>(row)];
        c.v.resize(static_cast<std::size_t>(n));
        double mx = 0.0;
        for (std::int64_t j = 0; j < n; ++j) { c.v[static_cast<std::size_t>(j)] = b * cols[p].v[static_cast<std::size_t>(j)] + a * cols[q].v[static_cast<std::size_t>(j)]; mx = std::max(mx, std::abs(c.v[static_cast<std::size_t>(j)])); }
        for (auto &x : c.v) { x /= mx; if (std::abs(x) < tolerance) x = 0.0; }
        c.v[static_cast<std::size_t>(row)] = 0.0;
        c.zero = common;
        fresh.push_back(std::move(c));
      }
    (void)kept;
    for (auto &c : fresh) next.push_back(std::move(c));
    dynamics::require(next.size() <= max_modes, "elementary_modes: intermediate mode count exceeded max_modes");
    cols = std::move(next);
  }
  // map back: permuted coordinate position -> split column -> original reaction with sign; drop 2-cycles (forward + backward of one reaction only)
  std::vector<double> out;
  std::int64_t count = 0;
  const auto rank_of_support = [&](const std::vector<std::int64_t> &support) { // rank of the split stoichiometry restricted to the support
    std::vector<std::vector<double>> a(static_cast<std::size_t>(m), std::vector<double>(support.size()));
    for (std::int64_t i = 0; i < m; ++i)
      for (std::size_t q = 0; q < support.size(); ++q) a[static_cast<std::size_t>(i)][q] = split_s[static_cast<std::size_t>(i)][static_cast<std::size_t>(perm[static_cast<std::size_t>(support[q])])];
    std::int64_t rk = 0;
    for (std::size_t col = 0; col < support.size() && rk < m; ++col) {
      std::int64_t piv = -1;
      double best = 1e-9;
      for (std::int64_t i = rk; i < m; ++i)
        if (std::abs(a[static_cast<std::size_t>(i)][col]) > best) { best = std::abs(a[static_cast<std::size_t>(i)][col]); piv = i; }
      if (piv < 0) continue;
      std::swap(a[static_cast<std::size_t>(piv)], a[static_cast<std::size_t>(rk)]);
      for (std::int64_t i = rk + 1; i < m; ++i) {
        const double f = a[static_cast<std::size_t>(i)][col] / a[static_cast<std::size_t>(rk)][col];
        for (std::size_t q = col; q < support.size(); ++q) a[static_cast<std::size_t>(i)][q] -= f * a[static_cast<std::size_t>(rk)][q];
      }
      ++rk;
    }
    return rk;
  };
  for (const auto &c : cols) {
    {
      std::vector<std::int64_t> support;
      for (std::int64_t p = 0; p < n; ++p)
        if (c.v[static_cast<std::size_t>(p)] > tolerance) support.push_back(p);
      if (support.empty() || rank_of_support(support) != static_cast<std::int64_t>(support.size()) - 1)
        continue; // not elementary (the combinatorial adjacency test is necessary but the rank test is the exact criterion)
    }
    std::vector<double> v(static_cast<std::size_t>(r), 0.0);
    std::vector<int> used(static_cast<std::size_t>(r), 0);
    std::int64_t support = 0;
    for (std::int64_t p = 0; p < n; ++p) {
      const double x = c.v[static_cast<std::size_t>(p)];
      if (x <= tolerance) continue;
      const std::int64_t split_col = perm[static_cast<std::size_t>(p)];
      const std::int64_t j = origin[static_cast<std::size_t>(split_col)];
      v[static_cast<std::size_t>(j)] += sign[static_cast<std::size_t>(split_col)] * x;
      ++used[static_cast<std::size_t>(j)];
      ++support;
    }
    bool cycle = false;
    for (std::int64_t j = 0; j < r; ++j)
      if (used[static_cast<std::size_t>(j)] == 2) cycle = true;
    if (cycle || support == 0)
      continue;
    double mx = 0.0;
    for (double x : v) mx = std::max(mx, std::abs(x));
    for (auto &x : v) x /= mx;
    // a mode built only from reversible reactions appears with both signs in the split network: keep one, first nonzero entry positive
    bool any_irreversible = false;
    for (std::int64_t j = 0; j < r; ++j)
      if (std::abs(v[static_cast<std::size_t>(j)]) > tolerance && (lo[static_cast<std::size_t>(j)] >= 0.0 || hi[static_cast<std::size_t>(j)] <= 0.0)) any_irreversible = true;
    if (!any_irreversible) {
      for (std::int64_t j = 0; j < r; ++j)
        if (std::abs(v[static_cast<std::size_t>(j)]) > tolerance) {
          if (v[static_cast<std::size_t>(j)] < 0.0) for (auto &x : v) x = -x;
          break;
        }
      bool duplicate = false;
      for (std::int64_t q = 0; q < count && !duplicate; ++q) {
        double dmax = 0.0;
        for (std::int64_t j = 0; j < r; ++j) dmax = std::max(dmax, std::abs(out[static_cast<std::size_t>(q * r + j)] - v[static_cast<std::size_t>(j)]));
        duplicate = dmax < 1e-7;
      }
      if (duplicate) continue;
    }
    out.insert(out.end(), v.begin(), v.end());
    ++count;
  }
  return pluss::make_series(out, {count, r}).to(pluss::ScalarType::Float64);
}

// ---- hit-and-run sampling ---------------------------------------------------------------------------------------------------------
struct SamplingOptions final {
  // Bounds with |b| >= infinity_threshold (including +-inf and sentinels such as 1e300) are treated as unbounded, and every unbounded side is truncated
  // at +-bound_cap (the COBRA convention of 1000), so the sampled polytope is always bounded; samples are then uniform on the TRUNCATED polytope.
  double bound_cap{1000.0};
  double infinity_threshold{1e20};
  // Reactions that the network forces to a single value (blocked reactions, implied equalities) are found by flux variability (range below this)
  // and held exactly at that value; without this the polytope has no interior in the full flux space and hit-and-run would be degenerate.
  double fixed_range_tolerance{1e-9};
};
struct Sample final {
  Series fluxes = pluss::series::undefined();      // [chains, samples, r]
  Series chebyshev_centre = pluss::series::undefined(); // [r]
  double chebyshev_radius{0.0};
  std::vector<std::int64_t> fixed_reactions{};     // reactions held at a single value (given l == u, or implied by the network)
  std::int64_t truncated_bounds{0};                // number of unbounded sides replaced by +-bound_cap
};
inline Sample sample_fluxes(const Network &net, std::int64_t chains, std::int64_t samples, std::int64_t thinning = 10, std::uint64_t seed = 0, const SamplingOptions &options = {}) {
  validate(net);
  dynamics::require(net.lower.dim() == 1 && chains > 0 && samples > 0 && thinning > 0, "sample_fluxes: bounds must be [r]; chains, samples, thinning positive");
  dynamics::require(options.bound_cap > 0.0 && options.infinity_threshold > 0.0, "sample_fluxes: bound_cap and infinity_threshold must be positive");
  const std::int64_t m = net.stoichiometry.size(0), r = net.stoichiometry.size(1);
  const Config cfg = Config{}.with_dtype(pluss::ScalarType::Float64);
  auto lo = net.lower.to(pluss::ScalarType::Float64).contiguous().to_vec<double>();
  auto hi = net.upper.to(pluss::ScalarType::Float64).contiguous().to_vec<double>();
  std::int64_t truncated = 0;
  for (std::int64_t j = 0; j < r; ++j) { // sentinels and infinities -> +-bound_cap
    auto &l = lo[static_cast<std::size_t>(j)];
    auto &u = hi[static_cast<std::size_t>(j)];
    if (l <= -options.infinity_threshold) { l = -options.bound_cap; ++truncated; }
    if (u >= options.infinity_threshold) { u = options.bound_cap; ++truncated; }
    dynamics::require(l <= u, "sample_fluxes: after truncation at bound_cap a lower bound exceeds its upper bound");
  }
  // reactions the network itself pins (blocked or implied equalities): one batched flux-variability solve on the truncated bounds
  Network capped = net;
  capped.lower = pluss::make_series(lo, {r}).to(pluss::ScalarType::Float64);
  capped.upper = pluss::make_series(hi, {r}).to(pluss::ScalarType::Float64);
  const auto fva = flux_variability(capped, pluss::series::zeros({r}, cfg), 0.0);
  const auto fmin = fva.minimum.contiguous().to_vec<double>(), fmax = fva.maximum.contiguous().to_vec<double>();
  std::vector<std::int64_t> fixed;
  std::vector<double> fixed_value(static_cast<std::size_t>(r), 0.0);
  for (std::int64_t j = 0; j < r; ++j) {
    const std::size_t q = static_cast<std::size_t>(j);
    if (fmax[q] - fmin[q] <= options.fixed_range_tolerance) {
      fixed.push_back(j);
      fixed_value[q] = 0.5 * (fmin[q] + fmax[q]);
      lo[q] = hi[q] = fixed_value[q];
    }
  }
  const std::int64_t nf = static_cast<std::int64_t>(fixed.size());
  // Chebyshev centre: max t s.t. S v = 0, v_j = l_j (fixed), v_j - t >= l_j, v_j + t <= u_j (free, finite bounds), t >= 0
  linprog::Problem p;
  std::vector<double> c(static_cast<std::size_t>(r + 1), 0.0);
  c[static_cast<std::size_t>(r)] = -1.0;
  p.c = pluss::make_series(c, {r + 1}).to(pluss::ScalarType::Float64);
  std::vector<double> aeq(static_cast<std::size_t>((m + nf) * (r + 1)), 0.0), beq(static_cast<std::size_t>(m + nf), 0.0);
  const auto s_host = net.stoichiometry.to(pluss::ScalarType::Float64).contiguous().to_vec<double>();
  for (std::int64_t i = 0; i < m; ++i)
    for (std::int64_t j = 0; j < r; ++j) aeq[static_cast<std::size_t>(i * (r + 1) + j)] = s_host[static_cast<std::size_t>(i * r + j)];
  for (std::int64_t k = 0; k < nf; ++k) { aeq[static_cast<std::size_t>((m + k) * (r + 1) + fixed[static_cast<std::size_t>(k)])] = 1.0; beq[static_cast<std::size_t>(m + k)] = fixed_value[static_cast<std::size_t>(fixed[static_cast<std::size_t>(k)])]; }
  std::vector<double> aub, bub;
  std::int64_t rows = 0;
  for (std::int64_t j = 0; j < r; ++j) {
    if (hi[static_cast<std::size_t>(j)] - lo[static_cast<std::size_t>(j)] < 1e-12) continue;
    if (std::isfinite(lo[static_cast<std::size_t>(j)])) { std::vector<double> row(static_cast<std::size_t>(r + 1), 0.0); row[static_cast<std::size_t>(j)] = -1.0; row[static_cast<std::size_t>(r)] = 1.0; aub.insert(aub.end(), row.begin(), row.end()); bub.push_back(-lo[static_cast<std::size_t>(j)]); ++rows; }
    if (std::isfinite(hi[static_cast<std::size_t>(j)])) { std::vector<double> row(static_cast<std::size_t>(r + 1), 0.0); row[static_cast<std::size_t>(j)] = 1.0; row[static_cast<std::size_t>(r)] = 1.0; aub.insert(aub.end(), row.begin(), row.end()); bub.push_back(hi[static_cast<std::size_t>(j)]); ++rows; }
  }
  p.a_eq = pluss::make_series(aeq, {m + nf, r + 1}).to(pluss::ScalarType::Float64);
  p.b_eq = pluss::make_series(beq, {m + nf}).to(pluss::ScalarType::Float64);
  p.a_ub = pluss::make_series(aub, {rows, r + 1}).to(pluss::ScalarType::Float64);
  p.b_ub = pluss::make_series(bub, {rows}).to(pluss::ScalarType::Float64);
  const double inf = std::numeric_limits<double>::infinity();
  std::vector<double> pl(static_cast<std::size_t>(r + 1), -inf), pu(static_cast<std::size_t>(r + 1), inf);
  pl[static_cast<std::size_t>(r)] = 0.0;
  p.lower = pluss::make_series(pl, {r + 1}).to(pluss::ScalarType::Float64);
  p.upper = pluss::make_series(pu, {r + 1}).to(pluss::ScalarType::Float64);
  const auto res = linprog::solve(p, exact_options());
  dynamics::require(res.status.item<double>() == static_cast<double>(linprog::Status::Optimal), "sample_fluxes: the Chebyshev-centre LP failed (infeasible network?)");
  dynamics::require(res.x.select(-1, r).item<double>() > 1e-9 || fixed.size() == static_cast<std::size_t>(r), "sample_fluxes: the flux polytope has no interior even after fixing the pinned reactions");
  const Series v0 = res.x.slice(-1, 0, r, 1);
  Sample out;
  out.chebyshev_centre = v0;
  out.chebyshev_radius = res.x.select(-1, r).item<double>();
  out.fixed_reactions = fixed;
  out.truncated_bounds = truncated;
  // nullspace of [S; fixed rows] by SVD
  std::vector<double> a(static_cast<std::size_t>((m + nf) * r), 0.0);
  for (std::int64_t i = 0; i < m; ++i)
    for (std::int64_t j = 0; j < r; ++j) a[static_cast<std::size_t>(i * r + j)] = s_host[static_cast<std::size_t>(i * r + j)];
  for (std::int64_t k = 0; k < nf; ++k) a[static_cast<std::size_t>((m + k) * r + fixed[static_cast<std::size_t>(k)])] = 1.0;
  const auto dec = pluss::subsp::linearalg::svd(pluss::make_series(a, {m + nf, r}).to(pluss::ScalarType::Float64), true);
  const auto sv = dec.s.contiguous().to_vec<double>();
  std::int64_t rank = 0;
  for (double s : sv) if (s > 1e-9 * std::max(1.0, sv.empty() ? 1.0 : sv.front())) ++rank;
  const std::int64_t d = r - rank;
  dynamics::require(d >= 1, "sample_fluxes: the feasible set is a single point");
  const Series basis = dec.vh.slice(0, rank, r, 1);                       // [d, r] orthonormal rows spanning the nullspace
  const Series lo_s = pluss::make_series(lo, {r}).to(pluss::ScalarType::Float64), hi_s = pluss::make_series(hi, {r}).to(pluss::ScalarType::Float64);
  const double big = 1e30;
  const Series lo_c = lo_s.clamp(-big, big), hi_c = hi_s.clamp(-big, big);
  Series v = v0.unsqueeze(0).expand({chains, r});
  const std::uint64_t base = pluss::tils::seed_or_next(seed);
  std::vector<Series> kept;
  std::uint64_t counter = 0;
  for (std::int64_t s = 0; s < samples * thinning; ++s) {
    const Series g = pluss::series::randn({chains, d}, cfg.dtype, base + (++counter));
    Series dir = g.matmul(basis);                                          // [chains, r]
    dir = dir.div(dir.square().sum(-1, true).sqrt());
    const Series small = dir.abs().lt(1e-12);
    const Series safe = small.where(dir.mul(0.0).add(1.0), dir);
    const Series t_hi = hi_c.sub(v).div(safe), t_lo = lo_c.sub(v).div(safe);     // roots of the two bounds along the line
    const Series upper_t = dir.gt(0.0).where(t_hi, t_lo), lower_t = dir.gt(0.0).where(t_lo, t_hi);
    const Series inf = dir.mul(0.0).add(1e300);
    const Series tmax = small.where(inf, upper_t).amin(-1, true), tmin = small.where(inf.neg(), lower_t).amax(-1, true);
    const Series u = pluss::series::rand({chains, 1}, cfg.dtype, base + (++counter));
    v = v.add(dir.mul(tmin.add(u.mul(tmax.sub(tmin)))));
    if ((s + 1) % thinning == 0)
      kept.push_back(v.unsqueeze(1));
  }
  std::vector<Series> rest(kept.begin() + 1, kept.end());
  out.fluxes = kept[0].unite(rest, 1);
  return out;
}

} // namespace pluss::subsp::natsci::biology::metabolism
