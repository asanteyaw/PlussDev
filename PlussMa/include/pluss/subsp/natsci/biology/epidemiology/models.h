#pragma once

// Compartmental epidemic models. States [..., n], compartments on the last
// axis, lanes in leading axes. Each model that appears in van den Driessche &
// Watmough (2002) uses their equation numbers and notation, and ships the
// F/V split from the paper so R0 = rho(F V^-1) can be recomputed numerically
// and compared with their closed forms.
//
//  sir / sis / seir : normalised or count states with standard incidence
//     beta S I / N (Hethcote, SIAM Rev. 42(4):599-653, 2000, sec. 2-3);
//     SEIR with demography uses births = deaths = mu N, so N is constant.
//  treatment        : vdD&W eq. (5), compartments (E, I, S, T), b(N) = d N.
//  staged           : vdD&W eq. (9), compartments (I_1..I_m, S).
//  multigroup       : vdD&W eq. (7), compartments (I_1..I_m, S_1..S_m, R_1..R_m).
//  multistrain      : vdD&W eq. (14), compartments (I1, I2, S).
//  vector_host      : vdD&W eq. (19), compartments (I, V, S, M).
//  final size       : Hethcote Thm 2.1 with the quotient ODE di/ds = -1 + 1/(sigma s).

#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/biology/epidemiology/compartmental.h"

namespace pluss::subsp::natsci::biology::epidemiology {

using dynamics::Coeff;

namespace detail {
inline Series join(const std::vector<Series> &parts) {
  std::vector<Series> tail;
  for (std::size_t i = 1; i < parts.size(); ++i)
    tail.push_back(parts[i].unsqueeze(-1));
  return parts.front().unsqueeze(-1).unite(tail, -1);
}
} // namespace detail

// ---- SIR / SIS / SEIR -----------------------------------------------------

// state (S, I, R); frequency-dependent incidence.
inline Series sir_rate(const Series &x, const Coeff &beta, const Coeff &gamma) {
  dynamics::require_species(x, 3, "sir: state");
  const Series s = x.select(-1, 0), i = x.select(-1, 1), r = x.select(-1, 2);
  const Series n = s.add(i).add(r);
  const Series infection = beta.on(s).mul(s).mul(i).div(n);
  const Series recovery = gamma.on(s).mul(i);
  return detail::join({infection.neg(), infection.sub(recovery), recovery});
}

// state (S, I): recovery returns to S.
inline Series sis_rate(const Series &x, const Coeff &beta, const Coeff &gamma) {
  dynamics::require_species(x, 2, "sis: state");
  const Series s = x.select(-1, 0), i = x.select(-1, 1);
  const Series infection = beta.on(s).mul(s).mul(i).div(s.add(i));
  const Series recovery = gamma.on(s).mul(i);
  return detail::join({recovery.sub(infection), infection.sub(recovery)});
}

// state (S, E, I, R); mu = per-capita birth = death rate (0 for an epidemic).
inline Series seir_rate(const Series &x, const Coeff &beta, const Coeff &sigma,
                        const Coeff &gamma, const Coeff &mu = 0.0) {
  dynamics::require_species(x, 4, "seir: state");
  const Series s = x.select(-1, 0), e = x.select(-1, 1), i = x.select(-1, 2),
               r = x.select(-1, 3);
  const Series n = s.add(e).add(i).add(r);
  const Series m = mu.on(s);
  const Series infection = beta.on(s).mul(s).mul(i).div(n);
  const Series ds = m.mul(n).sub(infection).sub(m.mul(s));
  const Series de = infection.sub(sigma.on(s).add(m).mul(e));
  const Series di = sigma.on(s).mul(e).sub(gamma.on(s).add(m).mul(i));
  const Series dr = gamma.on(s).mul(i).sub(m.mul(r));
  return detail::join({ds, de, di, dr});
}

// R0 of the SEIR model with demography: beta sigma / ((sigma+mu)(gamma+mu)).
inline Series seir_r0(const Series &beta, const Series &sigma,
                      const Series &gamma, const Series &mu) {
  return beta.mul(sigma).div(sigma.add(mu).mul(gamma.add(mu)));
}

// ---- Final size (Hethcote Theorem 2.1) --------------------------------------

// Initial replacement number sigma*s0 must exceed 1 for an epidemic. Solves
// g(s) = i0 + s0 - s + ln(s/s0)/sigma = 0 for the unique root in (0, 1/sigma)
// with the shared batched bisection (math/roots.h; g is increasing there).
inline Series final_susceptible(const Series &sigma, const Series &s0, const Series &i0,
                                std::int64_t iterations = 80) {
  dynamics::require_positive(sigma, "final_size: contact number");
  dynamics::require_positive(s0, "final_size: s0");
  dynamics::require_nonnegative(i0, "final_size: i0");
  namespace roots = ::pluss::subsp::math::roots;
  const Series lo = s0.mul(0.0).add(1e-300);
  const Series hi = sigma.reciprocal().mul(s0.mul(0.0).add(1.0));
  const auto g = [&](const Series &s) { return i0.add(s0).sub(s).add(s.div(s0).log().div(sigma)); };
  roots::Options opt;
  opt.max_iterations = iterations;
  return roots::bisect(g, lo, hi, opt).root;
}

// Peak infective fraction: i0 + s0 - 1/sigma - ln(sigma s0)/sigma.
inline Series peak_infective(const Series &sigma, const Series &s0,
                             const Series &i0) {
  return i0.add(s0).sub(sigma.reciprocal())
      .sub(sigma.mul(s0).log().div(sigma));
}

// ---- vdD&W treatment model, eq. (5) -------------------------------------------

struct TreatmentParams final {
  Coeff beta1{0.0}, beta2{0.0}, d{0.0}, nu{0.0}, r1{0.0}, r2{0.0}, q{0.0};
};

// state (E, I, S, T); b(N) = d N.
inline Series treatment_rate(const Series &x, const TreatmentParams &p) {
  dynamics::require_species(x, 4, "treatment: state");
  const Series e = x.select(-1, 0), i = x.select(-1, 1), s = x.select(-1, 2),
               t = x.select(-1, 3);
  const Series n = e.add(i).add(s).add(t);
  const Series d = p.d.on(e), nu = p.nu.on(e), r1 = p.r1.on(e), r2 = p.r2.on(e);
  const Series q = p.q.on(e);
  const Series pf = q.neg().add(1.0);
  const Series inf_s = p.beta1.on(e).mul(s).mul(i).div(n);
  const Series inf_t = p.beta2.on(e).mul(t).mul(i).div(n);
  const Series de = inf_s.add(inf_t).sub(d.add(nu).add(r1).mul(e)).add(pf.mul(r2).mul(i));
  const Series di = nu.mul(e).sub(d.add(r2).mul(i));
  const Series ds = d.mul(n).sub(d.mul(s)).sub(inf_s);
  const Series dt = d.neg().mul(t).add(r1.mul(e)).add(q.mul(r2).mul(i)).sub(inf_t);
  return detail::join({de, di, ds, dt});
}

inline Split treatment_split(const TreatmentParams &p) {
  Split sp;
  sp.infected = 2;
  sp.infections = [p](const Series &x) {
    const Series e = x.select(-1, 0), i = x.select(-1, 1), s = x.select(-1, 2),
                 t = x.select(-1, 3);
    const Series n = e.add(i).add(s).add(t);
    const Series f_e = p.beta1.on(e).mul(s).mul(i).div(n)
                           .add(p.beta2.on(e).mul(t).mul(i).div(n));
    const Series z = e.mul(0.0);
    return detail::join({f_e, z, z, z});
  };
  sp.transfers = [p](const Series &x) {
    const Series e = x.select(-1, 0), i = x.select(-1, 1), s = x.select(-1, 2),
                 t = x.select(-1, 3);
    const Series n = e.add(i).add(s).add(t);
    const Series d = p.d.on(e), nu = p.nu.on(e), r1 = p.r1.on(e), r2 = p.r2.on(e);
    const Series q = p.q.on(e);
    const Series pf = q.neg().add(1.0);
    const Series v_e = d.add(nu).add(r1).mul(e).sub(pf.mul(r2).mul(i));
    const Series v_i = nu.neg().mul(e).add(d.add(r2).mul(i));
    const Series v_s = d.mul(s).sub(d.mul(n)).add(p.beta1.on(e).mul(s).mul(i).div(n));
    const Series v_t = d.mul(t).sub(r1.mul(e)).sub(q.mul(r2).mul(i))
                           .add(p.beta2.on(e).mul(t).mul(i).div(n));
    return detail::join({v_e, v_i, v_s, v_t});
  };
  return sp;
}

// ---- vector-host model, eq. (19)-(21) -------------------------------------------

struct VectorHostParams final {
  Coeff beta_s{0.0}, beta_m{0.0}, b{0.0}, c{0.0}, gamma{0.0};
};

// state (I, V, S, M).
inline Series vector_host_rate(const Series &x, const VectorHostParams &p) {
  dynamics::require_species(x, 4, "vector_host: state");
  const Series i = x.select(-1, 0), v = x.select(-1, 1), s = x.select(-1, 2),
               m = x.select(-1, 3);
  const Series bs = p.beta_s.on(i), bm = p.beta_m.on(i), b = p.b.on(i),
               c = p.c.on(i), g = p.gamma.on(i);
  const Series di = bs.mul(s).mul(v).sub(b.add(g).mul(i));
  const Series dv = bm.mul(m).mul(i).sub(c.mul(v));
  const Series ds = b.sub(b.mul(s)).add(g.mul(i)).sub(bs.mul(s).mul(v));
  const Series dm = c.sub(c.mul(m)).sub(bm.mul(m).mul(i));
  return detail::join({di, dv, ds, dm});
}

inline Split vector_host_split(const VectorHostParams &p) {
  Split sp;
  sp.infected = 2;
  sp.infections = [p](const Series &x) {
    const Series i = x.select(-1, 0), v = x.select(-1, 1), s = x.select(-1, 2),
                 m = x.select(-1, 3);
    const Series z = i.mul(0.0);
    return detail::join({p.beta_s.on(i).mul(s).mul(v),
                         p.beta_m.on(i).mul(m).mul(i), z, z});
  };
  sp.transfers = [p](const Series &x) {
    const Series i = x.select(-1, 0), v = x.select(-1, 1), s = x.select(-1, 2),
                 m = x.select(-1, 3);
    const Series bs = p.beta_s.on(i), bm = p.beta_m.on(i), b = p.b.on(i),
                 c = p.c.on(i), g = p.gamma.on(i);
    return detail::join({b.add(g).mul(i), c.mul(v),
                         b.mul(s).sub(b).sub(g.mul(i)).add(bs.mul(s).mul(v)),
                         c.mul(m).sub(c).add(bm.mul(m).mul(i))});
  };
  return sp;
}

// ---- multistrain model, eq. (14) ----------------------------------------------------

struct MultistrainParams final {
  Coeff beta1{0.0}, beta2{0.0}, b{0.0}, gamma1{0.0}, gamma2{0.0}, nu{0.0};
};

// state (I1, I2, S).
inline Series multistrain_rate(const Series &x, const MultistrainParams &p) {
  dynamics::require_species(x, 3, "multistrain: state");
  const Series i1 = x.select(-1, 0), i2 = x.select(-1, 1), s = x.select(-1, 2);
  const Series b = p.b.on(s), nu = p.nu.on(s);
  const Series d1 = p.beta1.on(s).mul(i1).mul(s).sub(b.add(p.gamma1.on(s)).mul(i1))
                        .add(nu.mul(i1).mul(i2));
  const Series d2 = p.beta2.on(s).mul(i2).mul(s).sub(b.add(p.gamma2.on(s)).mul(i2))
                        .sub(nu.mul(i1).mul(i2));
  const Series ds = b.sub(b.mul(s)).add(p.gamma1.on(s).mul(i1))
                        .add(p.gamma2.on(s).mul(i2))
                        .sub(p.beta1.on(s).mul(i1).add(p.beta2.on(s).mul(i2)).mul(s));
  return detail::join({d1, d2, ds});
}

// ---- staged progression, eq. (9) --------------------------------------------------

// state (I_1..I_m, S); beta [m-1], nu [m-1], d [m] are Series on the last
// axis (or broadcastable); b is the scalar birth/death rate. The final stage
// has zero infectivity.
inline Series staged_rate(const Series &x, const Series &beta, const Series &nu,
                          const Series &d, const Coeff &b) {
  const std::int64_t m = x.size(-1) - 1;
  dynamics::require(m >= 2, "staged: need at least two infectious stages");
  dynamics::require(beta.size(-1) == m - 1 && nu.size(-1) == m - 1 && d.size(-1) == m,
                    "staged: beta/nu need m-1 entries and d needs m");
  const Series stages = x.slice(-1, 0, m, 1);
  const Series s = x.select(-1, m);
  const Series n = stages.sum(-1, false).add(s);
  const Series force = beta.mul(stages.slice(-1, 0, m - 1, 1)).sum(-1, false)
                           .mul(s).div(n);
  // I_1' = force - (nu_1 + d_1) I_1 ;  I_i' = nu_{i-1} I_{i-1} - (nu_i + d_i) I_i
  const Series nu_full = nu.unite(std::vector<Series>{nu.slice(-1, 0, 1, 1).mul(0.0)}, -1);
  const Series out = nu_full.add(d).mul(stages);
  const Series inflow = nu.mul(stages.slice(-1, 0, m - 1, 1));
  const Series lead = force.unsqueeze(-1);
  const Series shifted = lead.unite(std::vector<Series>{inflow}, -1);
  const Series di = shifted.sub(out);
  const Series ds = b.on(s).sub(b.on(s).mul(s)).sub(force);
  return di.unite(std::vector<Series>{ds.unsqueeze(-1)}, -1);
}

inline Split staged_split(const Series &beta, const Series &nu, const Series &d,
                          const Coeff &b) {
  Split sp;
  sp.infected = beta.size(-1) + 1;
  const std::int64_t m = sp.infected;
  sp.infections = [beta, m](const Series &x) {
    const Series stages = x.slice(-1, 0, m, 1);
    const Series s = x.select(-1, m);
    const Series n = stages.sum(-1, false).add(s);
    const Series force = beta.mul(stages.slice(-1, 0, m - 1, 1)).sum(-1, false)
                             .mul(s).div(n);
    const Series zeros = x.slice(-1, 1, m + 1, 1).mul(0.0);
    return force.unsqueeze(-1).unite(std::vector<Series>{zeros}, -1);
  };
  sp.transfers = [nu, d, b, m](const Series &x) {
    const Series stages = x.slice(-1, 0, m, 1);
    const Series s = x.select(-1, m);
    const Series nu_full =
        nu.unite(std::vector<Series>{nu.slice(-1, 0, 1, 1).mul(0.0)}, -1);
    const Series out = nu_full.add(d).mul(stages);
    const Series inflow = nu.mul(stages.slice(-1, 0, m - 1, 1));
    const Series lead = stages.slice(-1, 0, 1, 1).mul(0.0);
    const Series vi = out.sub(lead.unite(std::vector<Series>{inflow}, -1));
    // dS/dt = b - bS - force  =>  V_S = bS - b + force (force enters via F/V
    // split consistently only for infected rows; S row is not used for R0).
    const Series vs = b.on(s).mul(s).sub(b.on(s));
    return vi.unite(std::vector<Series>{vs.unsqueeze(-1)}, -1);
  };
  return sp;
}

inline Split multistrain_split(const MultistrainParams &p) {
  Split sp;
  sp.infected = 2;
  sp.infections = [p](const Series &x) {
    const Series i1 = x.select(-1, 0), i2 = x.select(-1, 1), s = x.select(-1, 2);
    const Series z = s.mul(0.0);
    return detail::join({p.beta1.on(s).mul(i1).mul(s) .add(p.nu.on(s).mul(i1).mul(i2)),
                         p.beta2.on(s).mul(i2).mul(s), z});
  };
  sp.transfers = [p](const Series &x) {
    const Series i1 = x.select(-1, 0), i2 = x.select(-1, 1), s = x.select(-1, 2);
    const Series b = p.b.on(s);
    return detail::join({b.add(p.gamma1.on(s)).mul(i1),
                         b.add(p.gamma2.on(s)).mul(i2).add(p.nu.on(s).mul(i1).mul(i2)),
                         s.mul(0.0)});
  };
  return sp;
}

} // namespace pluss::subsp::natsci::biology::epidemiology
