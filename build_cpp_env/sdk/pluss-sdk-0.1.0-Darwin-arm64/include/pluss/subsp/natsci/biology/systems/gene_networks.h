#pragma once

// Gene-regulatory circuits with Hill-type regulation. Dimensionless forms.
//
// repressilator: Elowitz & Leibler, Nature 403:335-338 (2000), Box 1 (read from
//   the paper's PDF), state (m1, m2, m3, p1, p2, p3):
//     dm_i = -m_i + alpha / (1 + p_j^n) + alpha0,   j = i-1 (cyclic ring)
//     dp_i = -beta (p_i - m_i)
//   beta is the PROTEIN-to-mRNA decay-RATE ratio in this equation (0.2 for
//   their 10 min / 2 min half-lives); Fig. 1b plots the inverse (lifetime
//   ratio, 5). Paper parameters: alpha = 216, alpha0/alpha = 1e-3, n = 2,
//   beta = 0.2 -> the steady state is linearly unstable and the ring
//   oscillates (verified here by eigenvalues and simulation).
// toggle_switch: Gardner, Cantor & Collins, Nature 403:339-342 (2000), eq. (1)
//   (read from the paper's PDF), state (u, v):
//     du = a1 / (1 + v^b) - u,   dv = a2 / (1 + u^g) - v
//   b, g = cooperativity of repression; nullclines are sigmoidal for b, g > 1
//   and the switch is bistable when synthesis is large enough and balanced.
// goodwin (Goodwin 1965; Griffith 1968 three-variable form), state (x, y, z):
//     dx = a / (1 + z^n) - b x,  dy = c x - b y,  dz = c y - b z
//   Hopf instability requires n > 8 (Gonze & Abou-Jaoude, PLoS ONE 2013).

#include "pluss/subsp/natsci/biology/dynamics.h"

namespace pluss::subsp::natsci::biology::systems::gene_networks {

using dynamics::Coeff;

namespace detail {
inline Series join_last(const std::vector<Series> &parts) {
  std::vector<Series> tail;
  for (std::size_t i = 1; i < parts.size(); ++i)
    tail.push_back(parts[i].unsqueeze(-1));
  return parts.front().unsqueeze(-1).unite(tail, -1);
}
} // namespace detail

inline Series repressilator_rate(const Series &s, const Coeff &alpha,
                                 const Coeff &alpha0, const Coeff &beta,
                                 const Coeff &n) {
  dynamics::require_species(s, 6, "repressilator: state");
  dynamics::require_nonnegative(s, "repressilator: state");
  const Series m = s.slice(-1, 0, 3, 1);
  const Series p = s.slice(-1, 3, 6, 1);
  // repressor of gene i is protein i-1 (cyclic): roll protein axis by one.
  const Series repressor =
      p.select(-1, 2).unsqueeze(-1).unite(
          std::vector<Series>{p.slice(-1, 0, 2, 1)}, -1);
  const Series dm = m.neg().add(
      alpha.on(s).unsqueeze(-1).div(repressor.pow(n.on(s).unsqueeze(-1)).add(1.0)))
                        .add(alpha0.on(s).unsqueeze(-1));
  const Series dp = p.sub(m).mul(beta.on(s).unsqueeze(-1)).neg();
  return dm.unite(std::vector<Series>{dp}, -1);
}

inline Series toggle_switch_rate(const Series &s, const Coeff &a1,
                                 const Coeff &a2, const Coeff &b,
                                 const Coeff &g) {
  dynamics::require_species(s, 2, "toggle_switch: state");
  const Series u = s.select(-1, 0);
  const Series v = s.select(-1, 1);
  const Series du = a1.on(u).div(v.pow(b.on(u)).add(1.0)).sub(u);
  const Series dv = a2.on(u).div(u.pow(g.on(u)).add(1.0)).sub(v);
  return detail::join_last({du, dv});
}

inline Series goodwin_rate(const Series &s, const Coeff &a, const Coeff &b,
                           const Coeff &c, const Coeff &n) {
  dynamics::require_species(s, 3, "goodwin: state");
  const Series x = s.select(-1, 0);
  const Series y = s.select(-1, 1);
  const Series z = s.select(-1, 2);
  const Series dx = a.on(x).div(z.pow(n.on(x)).add(1.0)).sub(b.on(x).mul(x));
  const Series dy = c.on(x).mul(x).sub(b.on(x).mul(y));
  const Series dz = c.on(x).mul(y).sub(b.on(x).mul(z));
  return detail::join_last({dx, dy, dz});
}

} // namespace pluss::subsp::natsci::biology::systems::gene_networks
