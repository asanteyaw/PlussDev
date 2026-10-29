#pragma once

// Gas-phase mechanism: Arrhenius, three-body and falloff (Lindemann/Troe)
// reactions with reverse rates from thermodynamics, batched over lanes
// (temperature [...] and concentrations [..., S]).
//
// Formulas follow Cantera's implementation (GitHub sources read):
//   k_f = A T^b exp(-Ea/(R T))                     (kinetics/Arrhenius.h)
//   three-body: k_f *= [M],  [M] = sum_k eff_k C_k  (default efficiency 1)
//   falloff:    k = k_inf Pr/(1+Pr) F,  Pr = k_0 [M]/k_inf   (Falloff.h)
//     Troe: Fcent = (1-a) exp(-T/T3) + a exp(-T/T1) [+ exp(-T2/T)],
//           c = -0.4 - 0.67 log10 Fcent,  n = 0.75 - 1.27 log10 Fcent,
//           f1 = (log10 Pr + c)/(n - 0.14 (log10 Pr + c)),
//           log10 F = log10 Fcent / (1 + f1^2)               (Falloff.cpp)
//   reverse:    k_r = k_f / Kc,  Kc = exp(-dG0/RT) (p0/RT)^dn (BulkKinetics.cpp
//               getEquilibriumConstants; standard concentration p0/RT)
// Internally SI (mol, m^3, s, J, K). Mechanism input may use the common
// CHEMKIN/Cantera unit set (cm, mol, s, cal/mol) via Units::cgs_cal().

#include "pluss/subsp/natsci/chemistry/thermo.h"
#include "pluss/subsp/natsci/common/reaction.h"

#include <map>

namespace pluss::subsp::natsci::chemistry::kinetics {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace reaction = ::pluss::subsp::natsci::common::reaction;
using thermo::SpeciesData;

struct Units final {
  double length_to_m{1.0};
  double energy_to_j_per_mol{1.0};
  static Units si() { return {}; }
  static Units cgs_cal() { return {1e-2, 4.184}; }
};

enum class Type { Elementary, ThreeBody, Falloff };

struct ReactionSpec final {
  std::map<std::string, double> reactants, products;
  double a{0.0}, b{0.0},
      ea{0.0}; // elementary / three-body, or high-pressure limit
  bool reversible{true};
  Type type{Type::Elementary};
  std::map<std::string, double> efficiencies; // third-body / falloff
  double default_efficiency{1.0};
  double a_low{0.0}, b_low{0.0}, ea_low{0.0}; // falloff low-pressure limit
  bool troe{false};
  std::array<double, 4> troe_params{}; // a, T3, T1, T2 (T2 <= 0: omitted)
};

class Mechanism final {
public:
  Mechanism(const std::vector<SpeciesData> &species,
            const std::vector<ReactionSpec> &reactions, const Series &like,
            const Units &units = Units::si())
      : thermo_(species, like) {
    const auto ns = static_cast<std::int64_t>(species.size());
    const auto nr = static_cast<std::int64_t>(reactions.size());
    dynamics::require(nr > 0, "kinetics: need at least one reaction");
    std::vector<double> molar_masses;
    molar_masses.reserve(species.size());
    bool any_molar_mass = false;
    bool all_molar_masses = true;
    for (const auto &s : species) {
      names_.push_back(s.name);
      any_molar_mass = any_molar_mass || s.molar_mass > 0.0;
      all_molar_masses =
          all_molar_masses && s.molar_mass > 0.0 && std::isfinite(s.molar_mass);
      molar_masses.push_back(s.molar_mass);
      for (const auto &e : s.composition)
        if (std::find(elements_.begin(), elements_.end(), e.first) ==
            elements_.end())
          elements_.push_back(e.first);
    }
    std::vector<double> stoich(nr * ns), fwd(nr * ns), rev(nr * ns),
        eff(nr * ns, 1.0);
    std::vector<double> log_a(nr), b(nr), ea_r(nr), log_a0(nr), b0(nr),
        ea_r0(nr);
    std::vector<double> reversible(nr), three(nr), falloff(nr), troe(nr),
        troe_p(nr * 4, 0.0);
    const double r_si = thermo::gas_constant();
    const auto index = [&](const std::string &name) {
      for (std::int64_t i = 0; i < ns; ++i)
        if (names_[static_cast<std::size_t>(i)] == name)
          return i;
      dynamics::require(false, "kinetics: unknown species '" + name + "'");
      return std::int64_t{0};
    };
    // CGS -> SI factor for the pre-exponential: (cm^3/mol)^(order-1) ->
    // (m^3/mol)^(order-1)
    const double volume = std::pow(units.length_to_m, 3.0);
    for (std::int64_t r = 0; r < nr; ++r) {
      const auto &rx = reactions[static_cast<std::size_t>(r)];
      double order = 0.0;
      for (const auto &[name, nu] : rx.reactants) {
        const auto i = index(name);
        stoich[r * ns + i] -= nu;
        fwd[r * ns + i] += nu;
        order += nu;
      }
      for (const auto &[name, nu] : rx.products) {
        const auto i = index(name);
        stoich[r * ns + i] += nu;
        rev[r * ns + i] += nu;
      }
      dynamics::require(
          rx.a > 0.0, "kinetics: pre-exponential must be positive (reaction " +
                          std::to_string(r) + ")");
      const double high_order =
          order + (rx.type == Type::ThreeBody ? 1.0 : 0.0);
      log_a[r] = std::log(rx.a * std::pow(volume, high_order - 1.0));
      b[r] = rx.b;
      ea_r[r] = rx.ea * units.energy_to_j_per_mol / r_si;
      reversible[r] = rx.reversible ? 1.0 : 0.0;
      three[r] = rx.type == Type::ThreeBody ? 1.0 : 0.0;
      falloff[r] = rx.type == Type::Falloff ? 1.0 : 0.0;
      if (rx.type != Type::Elementary) {
        for (std::int64_t i = 0; i < ns; ++i)
          eff[r * ns + i] = rx.default_efficiency;
        for (const auto &[name, e] : rx.efficiencies)
          eff[r * ns + index(name)] = e;
      }
      if (rx.type == Type::Falloff) {
        dynamics::require(rx.a_low > 0.0,
                          "kinetics: falloff needs a low-pressure limit");
        log_a0[r] = std::log(rx.a_low * std::pow(volume, order + 1.0 - 1.0));
        b0[r] = rx.b_low;
        ea_r0[r] = rx.ea_low * units.energy_to_j_per_mol / r_si;
        troe[r] = rx.troe ? 1.0 : 0.0;
        for (int k = 0; k < 4; ++k)
          troe_p[r * 4 + k] = rx.troe_params[static_cast<std::size_t>(k)];
        // falloff high-limit uses the un-augmented order (no M)
      }
    }
    const auto cast = [&](Series v) {
      return v.to(like.scalar_type()).to(like.platform());
    };
    const auto mat = [&](const std::vector<double> &v, std::int64_t rows,
                         std::int64_t cols) {
      return cast(pluss::make_series(v, {rows, cols}));
    };
    const auto vec = [&](const std::vector<double> &v) {
      return cast(pluss::make_series(v, {nr}));
    };
    const auto vec2 = [&](const std::vector<double> &v) {
      return cast(pluss::make_series(v, {static_cast<std::int64_t>(v.size())}));
    };
    dynamics::require(!any_molar_mass || all_molar_masses,
                      "kinetics: molar masses must be finite and positive for "
                      "either every species or none");
    if (all_molar_masses)
      molar_masses_ = vec2(molar_masses);
    stoich_ = mat(stoich, nr, ns);
    std::vector<double> both(fwd);
    both.insert(both.end(), rev.begin(), rev.end());
    orders_both_t_ =
        mat(both, 2 * nr, ns).transpose(0, 1); // [S, 2R]: forward then reverse
    efficiencies_ = mat(eff, nr, ns);
    log_a_ = vec(log_a);
    b_ = vec(b);
    ea_r_ = vec(ea_r);
    log_a0_ = vec(log_a0);
    b0_ = vec(b0);
    ea_r0_ = vec(ea_r0);
    reversible_ = vec(reversible);
    three_ = vec(three);
    falloff_ = vec(falloff);
    troe_ = vec(troe);
    troe_p_ = mat(troe_p, nr, 4);
    // Falloff reactions are a (usually tiny) subset: their Pr / Troe work is
    // done on just those columns and scattered back with a 0/1 matrix [nf, R].
    std::vector<std::int64_t> fo;
    for (std::int64_t r = 0; r < nr; ++r)
      if (falloff[static_cast<std::size_t>(r)] > 0.0)
        fo.push_back(r);
    n_falloff_ = static_cast<std::int64_t>(fo.size());
    if (n_falloff_ > 0) {
      std::vector<double> sel(static_cast<std::size_t>(n_falloff_ * nr), 0.0),
          tp(static_cast<std::size_t>(n_falloff_ * 4)),
          flag(static_cast<std::size_t>(n_falloff_));
      std::vector<double> la(static_cast<std::size_t>(n_falloff_)),
          bb(static_cast<std::size_t>(n_falloff_)),
          ee(static_cast<std::size_t>(n_falloff_));
      std::vector<double> ef(static_cast<std::size_t>(n_falloff_ * ns));
      for (std::int64_t k = 0; k < n_falloff_; ++k) {
        const auto r = fo[static_cast<std::size_t>(k)];
        sel[static_cast<std::size_t>(k * nr + r)] = 1.0;
        for (int j = 0; j < 4; ++j)
          tp[static_cast<std::size_t>(k * 4 + j)] =
              troe_p[static_cast<std::size_t>(r * 4 + j)];
        flag[static_cast<std::size_t>(k)] = troe[static_cast<std::size_t>(r)];
        la[static_cast<std::size_t>(k)] = log_a0[static_cast<std::size_t>(r)];
        bb[static_cast<std::size_t>(k)] = b0[static_cast<std::size_t>(r)];
        ee[static_cast<std::size_t>(k)] = ea_r0[static_cast<std::size_t>(r)];
        for (std::int64_t i = 0; i < ns; ++i)
          ef[static_cast<std::size_t>(k * ns + i)] =
              eff[static_cast<std::size_t>(r * ns + i)];
      }
      fall_select_ = mat(sel, n_falloff_, nr);
      fall_troe_ = mat(tp, n_falloff_, 4);
      fall_is_troe_ = vec2(flag);
      fall_log_a0_ = vec2(la);
      fall_b0_ = vec2(bb);
      fall_ea_r0_ = vec2(ee);
      fall_eff_ = mat(ef, n_falloff_, ns);
      fall_idx_ = pluss::make_series(fo, {n_falloff_})
                      .to(pluss::ScalarType::Int64)
                      .to(like.platform());
    }
    std::vector<double> comp(ns * static_cast<std::int64_t>(elements_.size()),
                             0.0);
    for (std::int64_t i = 0; i < ns; ++i)
      for (const auto &e : species[static_cast<std::size_t>(i)].composition)
        comp[i * static_cast<std::int64_t>(elements_.size()) +
             static_cast<std::int64_t>(
                 std::find(elements_.begin(), elements_.end(), e.first) -
                 elements_.begin())] = e.second;
    composition_ = mat(comp, ns, static_cast<std::int64_t>(elements_.size()));
  }

  [[nodiscard]] const thermo::Nasa7 &thermo() const { return thermo_; }
  [[nodiscard]] std::int64_t species() const { return stoich_.size(1); }
  [[nodiscard]] std::int64_t reactions() const { return stoich_.size(0); }
  [[nodiscard]] const std::vector<std::string> &names() const { return names_; }
  [[nodiscard]] const std::vector<std::string> &elements() const {
    return elements_;
  }
  [[nodiscard]] const Series &stoichiometry() const { return stoich_; }
  [[nodiscard]] const Series &composition() const { return composition_; }
  [[nodiscard]] bool has_molar_masses() const noexcept {
    return molar_masses_.defined();
  }
  [[nodiscard]] const Series &molar_masses() const {
    dynamics::require(has_molar_masses(),
                      "kinetics: this mechanism has no molar-mass data");
    return molar_masses_;
  }
  [[nodiscard]] std::int64_t index(const std::string &name) const {
    for (std::size_t i = 0; i < names_.size(); ++i)
      if (names_[i] == name)
        return static_cast<std::int64_t>(i);
    dynamics::require(false, "kinetics: unknown species '" + name + "'");
    return 0;
  }

  // Third-body concentration [M] per reaction: [..., R].
  [[nodiscard]] Series third_body(const Series &c) const {
    return dynamics::matvec(efficiencies_, c);
  }

  // Forward rate constants [..., R] (SI; includes [M] for three-body
  // reactions).
  [[nodiscard]] Series forward_rate_constants(const Series &t,
                                              const Series &c) const {
    const Series temp = t.unsqueeze(-1);
    const Series log_t = temp.log();
    const Series inv_t = temp.reciprocal();
    const Series k_main = log_a_.add(b_.mul(log_t)).sub(ea_r_.mul(inv_t)).exp();
    const Series m = third_body(c);
    // elementary x1, three-body x[M], falloff handled separately below
    const Series factor =
        three_.mul(m.sub(1.0)).add(1.0).mul(falloff_.neg().add(1.0));
    Series k = k_main.mul(factor);
    if (n_falloff_ > 0) {
      const Series kinf = k_main.index_select(-1, fall_idx_); // [..., nf]
      const Series mf = dynamics::matvec(fall_eff_, c);       // [..., nf]
      const Series k0 = fall_log_a0_.add(fall_b0_.mul(log_t))
                            .sub(fall_ea_r0_.mul(inv_t))
                            .exp();
      const Series pr = k0.mul(mf).div(kinf);
      // Troe broadening (Lindemann rows: F = 1)
      const Series a = fall_troe_.select(-1, 0), t3 = fall_troe_.select(-1, 1),
                   t1 = fall_troe_.select(-1, 2), t2 = fall_troe_.select(-1, 3);
      Series fcent =
          a.neg()
              .add(1.0)
              .mul(temp.neg().div(t3.clamp(1e-30, 1e300)).exp())
              .add(a.mul(temp.neg().div(t1.clamp(1e-30, 1e300)).exp()));
      fcent = fcent.add(
          t2.gt(0.0).to(t.scalar_type()).mul(t2.neg().mul(inv_t).exp()));
      const Series log_fc =
          fcent.clamp(1e-300, 1e300).log().div(std::log(10.0));
      const Series cc = log_fc.mul(-0.67).sub(0.4);
      const Series nn = log_fc.mul(-1.27).add(0.75);
      const Series lpr = pr.clamp(1e-300, 1e300).log().div(std::log(10.0));
      const Series f1 = lpr.add(cc).div(nn.sub(lpr.add(cc).mul(0.14)));
      const Series f_troe =
          log_fc.div(f1.square().add(1.0)).mul(std::log(10.0)).exp();
      const Series f =
          fall_is_troe_.gt(0.0).where(f_troe, f_troe.mul(0.0).add(1.0));
      const Series k_fall = kinf.mul(pr.div(pr.add(1.0))).mul(f); // [..., nf]
      k = k.add(k_fall.matmul(fall_select_)); // scatter to [..., R]
    }
    return k;
  }

  // Equilibrium constants in concentration units: [..., R].
  [[nodiscard]] Series equilibrium_constants(const Series &t) const {
    const Series dg_rt = dynamics::matvec(stoich_, thermo_.g_rt(t));
    const Series dn = stoich_.sum(-1, false);
    const Series log_c0 = t.mul(thermo::gas_constant())
                              .reciprocal()
                              .mul(thermo::standard_pressure)
                              .log()
                              .unsqueeze(-1);
    return dg_rt.neg().add(dn.mul(log_c0)).exp();
  }

  struct Progress final {
    Series forward = pluss::series::undefined();
    Series reverse = pluss::series::undefined();
  };

  [[nodiscard]] Progress rates_of_progress(const Series &t,
                                           const Series &c) const {
    return rates_of_progress(t, c, thermo_.eval(t));
  }
  // Same, reusing already-evaluated thermodynamic properties at t.
  [[nodiscard]] Progress
  rates_of_progress(const Series &t, const Series &c,
                    const thermo::Nasa7::Properties &props) const {
    const Series cc = c.clamp(0.0, 1e300);
    const Series kf = forward_rate_constants(t, cc);
    // 1/Kc computed directly (Cantera: m_rkcn = exp(dG0/RT - dn ln c0)),
    // bounded.
    const Series dg_rt = dynamics::matvec(stoich_, props.h_rt.sub(props.s_r));
    const Series dn = stoich_.sum(-1, false);
    const Series log_c0 = t.mul(thermo::gas_constant())
                              .reciprocal()
                              .mul(thermo::standard_pressure)
                              .log()
                              .unsqueeze(-1);
    const Series rkc = dg_rt.sub(dn.mul(log_c0)).clamp(-700.0, 700.0).exp();
    const Series kr = kf.mul(rkc).mul(reversible_);
    // forward and reverse concentration products in one log-space matmul
    const Series products =
        reaction::positive_products(orders_both_t_, cc); // [..., 2R]
    const std::int64_t nr = stoich_.size(0);
    return {products.slice(-1, 0, nr, 1).mul(kf),
            products.slice(-1, nr, 2 * nr, 1).mul(kr)};
  }

  // Net production rates omega_k [..., S] in mol m^-3 s^-1.
  [[nodiscard]] Series net_production_rates(const Series &t,
                                            const Series &c) const {
    const auto q = rates_of_progress(t, c);
    return q.forward.sub(q.reverse).matmul(stoich_);
  }
  [[nodiscard]] Series
  net_production_rates(const Series &t, const Series &c,
                       const thermo::Nasa7::Properties &props) const {
    const auto q = rates_of_progress(t, c, props);
    return q.forward.sub(q.reverse).matmul(stoich_);
  }

private:
  thermo::Nasa7 thermo_;
  std::vector<std::string> names_, elements_;
  Series stoich_ = pluss::series::undefined();
  Series orders_both_t_ = pluss::series::undefined(); // [S, 2R]
  Series efficiencies_ = pluss::series::undefined();
  Series log_a_ = pluss::series::undefined(), b_ = pluss::series::undefined(),
         ea_r_ = pluss::series::undefined();
  Series log_a0_ = pluss::series::undefined(), b0_ = pluss::series::undefined(),
         ea_r0_ = pluss::series::undefined();
  Series reversible_ = pluss::series::undefined(),
         three_ = pluss::series::undefined(),
         falloff_ = pluss::series::undefined(),
         troe_ = pluss::series::undefined();
  Series troe_p_ = pluss::series::undefined();
  std::int64_t n_falloff_{0};
  Series fall_select_ = pluss::series::undefined(),
         fall_troe_ = pluss::series::undefined(),
         fall_is_troe_ = pluss::series::undefined(),
         fall_log_a0_ = pluss::series::undefined(),
         fall_b0_ = pluss::series::undefined(),
         fall_ea_r0_ = pluss::series::undefined(),
         fall_eff_ = pluss::series::undefined(),
         fall_idx_ = pluss::series::undefined();
  Series composition_ = pluss::series::undefined();
  Series molar_masses_ = pluss::series::undefined();
};

} // namespace pluss::subsp::natsci::chemistry::kinetics
