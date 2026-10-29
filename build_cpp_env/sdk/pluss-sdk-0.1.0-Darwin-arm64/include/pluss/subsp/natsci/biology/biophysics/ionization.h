#pragma once

// Acid-base ionization of macromolecules and small polyprotic molecules, batched over lanes (sequences, pH values).
//
// Each ionizable group g has a pK_a and a sign s_g (+1: a base, charged when protonated; -1: an acid, charged when deprotonated). With
// independent groups the Henderson-Hasselbalch fraction in the charged form is 1 / (1 + 10^{s_g (pH - pK_g)}) and
//
//     Q(pH)    = sum_g n_g s_g / (1 + 10^{s_g (pH - pK_g)})          (net charge, n_g = number of groups of type g)
//     beta(pH) = ln 10 sum_g n_g f_g (1 - f_g)                       (Van Slyke buffer capacity of the groups, mol per pH unit per mol)
//     pI       : Q(pI) = 0 (Q decreases strictly with pH, so the root is unique; found by bisection on all lanes together)
//
// `protein_groups` builds the group table of the Bjellqvist method as implemented in Biopython's Bio/SeqUtils/IsoelectricPoint.py (read from
// the GitHub source, sha 93a0d329ba63): side chains K 10.0, R 12.0, H 5.98 (bases), D 4.05, E 4.45, C 9.0, Y 10.0 (acids), termini
// N 7.5 / C 3.55 with the sequence-specific overrides of the original tables (Bjellqvist et al., Electrophoresis 14 (1993) 1023, not read).

#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>
#include <string>
#include <vector>

namespace pluss::subsp::natsci::biology::biophysics::ionization {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;

// counts [..., G], pka [..., G] or [G], sign [G]; pH any shape broadcasting against the lanes through an extra trailing axis handled by the callers
namespace detail {
inline Series charged_fraction(const Series &ph, const Series &pka, const Series &sign) { // ph [..., 1], result [..., G]
  return ph.sub(pka).mul(sign).mul(std::log(10.0)).exp().add(1.0).reciprocal();
}
} // namespace detail

// net charge at pH [..., 1] -> [...]; counts [..., G]
inline Series charge(const Series &counts, const Series &pka, const Series &sign, const Series &ph) {
  const Series f = detail::charged_fraction(ph.unsqueeze(-1), pka, sign);
  return f.mul(sign).mul(counts).sum(-1, false);
}

// Van Slyke buffer capacity of the ionizable groups
inline Series buffer_capacity(const Series &counts, const Series &pka, const Series &sign, const Series &ph) {
  const Series f = detail::charged_fraction(ph.unsqueeze(-1), pka, sign);
  return f.mul(f.neg().add(1.0)).mul(counts).sum(-1, false).mul(std::log(10.0));
}

// Isoelectric point by bisection of the monotone charge on every lane at once (`iterations` halvings of [lo, hi]).
inline Series isoelectric_point(const Series &counts, const Series &pka, const Series &sign, double lo = -2.0, double hi = 16.0, int iterations = 60) {
  dynamics::require(lo < hi && iterations > 0, "isoelectric_point: need lo < hi and a positive iteration count");
  const Series lane = counts.sum(-1, false);
  Series a = lane.mul(0.0).add(lo), b = lane.mul(0.0).add(hi);
  for (int i = 0; i < iterations; ++i) {
    const Series mid = a.add(b).mul(0.5);
    const Series positive = charge(counts, pka, sign, mid).gt(0.0);
    a = positive.where(mid, a);
    b = positive.where(b, mid);
  }
  return a.add(b).mul(0.5);
}

struct Groups final {
  Series counts = pluss::series::undefined(); // [N, 9]  order: Nterm, Cterm, K, R, H, D, E, C, Y
  Series pka = pluss::series::undefined();    // [N, 9]  sequence-specific termini
  Series sign = pluss::series::undefined();   // [9]
};

inline Groups protein_groups(const std::vector<std::string> &sequences) {
  static const char residues[7] = {'K', 'R', 'H', 'D', 'E', 'C', 'Y'};
  const auto n_term_pk = [](char c) {
    switch (c) {
    case 'A': return 7.59;
    case 'M': return 7.0;
    case 'S': return 6.93;
    case 'P': return 8.36;
    case 'T': return 6.82;
    case 'V': return 7.44;
    case 'E': return 7.7;
    default: return 7.5;
    }
  };
  const auto c_term_pk = [](char c) { return c == 'D' ? 4.55 : c == 'E' ? 4.75 : 3.55; };
  const double side_pk[7] = {10.0, 12.0, 5.98, 4.05, 4.45, 9.0, 10.0};
  std::vector<double> counts, pka;
  for (const std::string &raw : sequences) {
    dynamics::require(!raw.empty(), "protein_groups: empty sequence");
    std::string s;
    for (char c : raw)
      s.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    counts.push_back(1.0);
    counts.push_back(1.0);
    for (char r : residues) {
      double k = 0.0;
      for (char c : s)
        k += (c == r) ? 1.0 : 0.0;
      counts.push_back(k);
    }
    pka.push_back(n_term_pk(s.front()));
    pka.push_back(c_term_pk(s.back()));
    for (double p : side_pk)
      pka.push_back(p);
  }
  const auto n = static_cast<std::int64_t>(sequences.size());
  Groups g;
  g.counts = pluss::make_series(counts, {n, 9}).to(pluss::ScalarType::Float64);
  g.pka = pluss::make_series(pka, {n, 9}).to(pluss::ScalarType::Float64);
  g.sign = pluss::make_series(std::vector<double>{1, -1, 1, 1, 1, -1, -1, -1, -1}, {9}).to(pluss::ScalarType::Float64);
  return g;
}

} // namespace pluss::subsp::natsci::biology::biophysics::ionization
