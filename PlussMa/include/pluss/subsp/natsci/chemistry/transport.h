#pragma once

// Gas-phase transport properties from Chapman-Enskog kinetic theory, batched
// over temperature and composition lanes (T [...], mole fractions [..., S]).
// Formulas as implemented in Cantera (src/transport/GasTransport.cpp and
// MixTransport.cpp, read from GitHub), which follow the CHEMKIN transport
// formulation (Kee, Dixon-Lewis, Warnatz, Coltrin, Miller):
//   eta_k    = (5/16) sqrt(pi m_k kB T) / (pi sigma_k^2 Omega22*(T*)) T* = kB T
//   / eps D_jk p   = (3/16) sqrt(2 pi / m_jk) (kB T)^(3/2) / (pi sigma_jk^2
//   Omega11*)  (reduced mass m_jk) lambda_k = (eta_k / M_k) R (1.5 f_trans +
//   c_rot f_rot + c_v,int f_int)       (modified Eucken with
//              rotational relaxation: f_z(T*) = 1 + pi^1.5/sqrt(T*) (0.5 +
//              1/T*) + (pi^2/4 + 2)/T*)
//   mixtures: Wilke viscosity (phi_kj of Poling, eq. 9-5.14),
//   arithmetic-harmonic conductivity
//             0.5 (sum x lambda + 1/sum(x/lambda)), mixture-averaged diffusion
//             D_k = (1 - x_k) / (p sum_{j != k} x_j / D_jk).
// Collision integrals: the analytic Neufeld-Janzen-Aziz fits of the
// Lennard-Jones (12-6) reduced integrals. Cantera instead interpolates the
// Monchick-Mason tables, so values agree to a few tenths of a percent, not
// machine precision. Polar species (non-zero dipole) are rejected: the
// Stockmayer correction is not implemented here. Units: SI throughout (m, kg,
// mol, s, K, Pa, J).

#include "pluss/subsp/natsci/chemistry/thermo.h"
#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>
#include <numbers>
#include <string>
#include <vector>

namespace pluss::subsp::natsci::chemistry::transport {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
namespace constants = ::pluss::subsp::natsci::common::constants::codata2022;

inline double boltzmann() {
  return constants::boltzmann_constant.measurement.value().si_value();
}
inline double avogadro() {
  return constants::avogadro_constant.measurement.value().si_value();
}

struct SpeciesData final {
  std::string name;
  int geometry{0};          // 0 atom, 1 linear, 2 non-linear
  double well_depth_k{0.0}; // epsilon / kB [K]
  double diameter_angstrom{0.0};
  double dipole{0.0}; // C m; must be 0 (polar species unsupported)
  double rotational_relaxation{0.0};
  double molar_mass{0.0}; // kg/mol
};

// Reduced collision integrals (Neufeld, Janzen & Aziz, J. Chem. Phys. 57, 1100
// (1972) analytic fits).
inline Series omega22(const Series &tstar) {
  const Series t = tstar;
  const Series arg = t.pow(-0.7683).mul(18.0323).sub(7.27371);
  return t.pow(-0.14874)
      .mul(1.16145)
      .add(t.mul(-0.77320).exp().mul(0.52487))
      .add(t.mul(-2.43787).exp().mul(2.16178))
      .sub(t.pow(0.14874).mul(arg.sin()).mul(6.435e-4));
}
inline Series omega11(const Series &tstar) {
  const Series t = tstar;
  return t.pow(-0.15610)
      .mul(1.06036)
      .add(t.mul(-0.47635).exp().mul(0.19300))
      .add(t.mul(-1.52996).exp().mul(1.03587))
      .add(t.mul(-3.89411).exp().mul(1.76474));
}

class GasTransport final {
public:
  GasTransport(const std::vector<SpeciesData> &species, const Series &like) {
    const auto n = static_cast<std::int64_t>(species.size());
    dynamics::require(n > 0, "transport: need at least one species");
    std::vector<double> eps(n), sigma(n), mass(n), crot(n), zrot(n);
    for (std::int64_t i = 0; i < n; ++i) {
      const auto &s = species[static_cast<std::size_t>(i)];
      dynamics::require(s.dipole == 0.0, "transport: polar species (" + s.name +
                                             ") are not supported");
      dynamics::require(s.well_depth_k > 0.0 && s.diameter_angstrom > 0.0 &&
                            s.molar_mass > 0.0,
                        "transport: invalid Lennard-Jones data for " + s.name);
      names_.push_back(s.name);
      eps[i] = s.well_depth_k;
      sigma[i] = s.diameter_angstrom * 1e-10;
      mass[i] = s.molar_mass;
      crot[i] = s.geometry == 0 ? 0.0 : (s.geometry == 1 ? 1.0 : 1.5);
      zrot[i] = s.rotational_relaxation;
    }
    const auto vec = [&](const std::vector<double> &v) {
      return pluss::make_series(v, {n})
          .to(like.scalar_type())
          .to(like.platform());
    };
    eps_ = vec(eps);
    sigma_ = vec(sigma);
    mass_ = vec(mass);
    crot_ = vec(crot);
    zrot_ = vec(zrot);
    // pair constants (cached once): reduced mass, combined diameter / well
    // depth, Wilke mass factors
    const Series mi = mass_.unsqueeze(-1), mj = mass_.unsqueeze(0);
    reduced_mass_ =
        mi.mul(mj).div(mi.add(mj)).div(avogadro()); // kg per molecule
    sigma_pair_ = sigma_.unsqueeze(-1).add(sigma_.unsqueeze(0)).mul(0.5);
    eps_pair_ = eps_.unsqueeze(-1).mul(eps_.unsqueeze(0)).sqrt(); // K
    wilke_mass_ = mj.div(mi).pow(0.25); // (M_j/M_k)^(1/4), row k column j
    wilke_inv_ = mi.div(mj).add(1.0).sqrt().mul(std::sqrt(8.0)).reciprocal();
    eye_ = pluss::series::eye(
        n,
        Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
  }

  [[nodiscard]] std::int64_t species() const { return mass_.size(0); }
  [[nodiscard]] const std::vector<std::string> &names() const { return names_; }
  [[nodiscard]] const Series &molar_masses() const { return mass_; }

  // Composition conversions use a trailing species axis and retain every
  // leading batch/spatial axis. Keeping them here ensures every transport
  // consumer uses the same cached molar masses and normalization convention.
  [[nodiscard]] Series mole_fractions(const Series &mass_fractions) const {
    dynamics::require(mass_fractions.dim() > 0 &&
                          mass_fractions.size(-1) == species(),
                      "transport: mass fractions need a trailing species "
                      "axis matching the transport model");
    const Series moles = mass_fractions.div(mass_);
    return moles.div(moles.sum(-1, true));
  }

  [[nodiscard]] Series mass_fractions(const Series &mole_fractions) const {
    dynamics::require(mole_fractions.dim() > 0 &&
                          mole_fractions.size(-1) == species(),
                      "transport: mole fractions need a trailing species "
                      "axis matching the transport model");
    const Series mass = mole_fractions.mul(mass_);
    return mass.div(mass.sum(-1, true));
  }

  // Binary diffusion products D_jk p [Pa m^2/s]. This is separated from the
  // full species-property evaluation so diffusion-only solvers do not compute
  // viscosity and conductivity that they will discard.
  [[nodiscard]] Series binary_diffusion(const Series &t) const {
    const double kb = boltzmann();
    const Series temp = t.unsqueeze(-1).unsqueeze(-1);
    const Series tstar_pair = temp.div(eps_pair_);
    const Series kt = temp.mul(kb);
    return kt.pow(1.5)
        .mul(reduced_mass_.reciprocal().mul(2.0 * std::numbers::pi_v<double>).sqrt())
        .div(omega11(tstar_pair).mul(sigma_pair_.square().mul(std::numbers::pi_v<double>)))
        .mul(3.0 / 16.0);
  }

  struct Species final {
    Series viscosity = pluss::series::undefined();    // [..., S] Pa s
    Series conductivity = pluss::series::undefined(); // [..., S] W/m/K
    Series binary_diffusion =
        pluss::series::undefined(); // [..., S, S] m^2/s at 1 Pa (divide by p)
  };

  // cp_over_r: [..., S] from thermo::Nasa7::eval(T).cp_r.
  [[nodiscard]] Species species_properties(const Series &t,
                                           const Series &cp_over_r) const {
    const double kb = boltzmann(), r = thermo::gas_constant();
    const Series temp = t.unsqueeze(-1);    // [..., 1]
    const Series tstar = temp.div(eps_);    // [..., S]
    const Series m = mass_.div(avogadro()); // kg per molecule [S]
    const Series kt = temp.mul(kb);
    const Series pi_sigma2 = sigma_.square().mul(std::numbers::pi_v<double>);
    const Series eta = m.mul(kt)
                           .mul(std::numbers::pi_v<double>)
                           .sqrt()
                           .div(omega22(tstar).mul(pi_sigma2))
                           .mul(5.0 / 16.0);
    // self diffusion D_kk p and the internal-energy factors
    const Series d_self =
        kt.pow(1.5)
            .mul(m.mul(0.5).reciprocal().mul(2.0 * std::numbers::pi_v<double>).sqrt())
            .div(omega11(tstar).mul(pi_sigma2))
            .mul(3.0 / 16.0);
    const Series f_int = mass_.div(temp.mul(r)).mul(d_self).div(eta);
    const auto fz = [](const Series &ts) {
      return ts.sqrt()
          .reciprocal()
          .mul(std::pow(std::numbers::pi_v<double>, 1.5))
          .mul(ts.reciprocal().add(0.5))
          .add(1.0)
          .add(ts.reciprocal().mul(std::numbers::pi_v<double> * std::numbers::pi_v<double> / 4.0 + 2.0));
    };
    const Series fz298 = fz(eps_.reciprocal().mul(298.0));
    const Series a = f_int.neg().add(2.5);
    const Series b = zrot_.mul(fz298).div(fz(tstar)).add(
        f_int.add(crot_.mul(5.0 / 3.0)).mul(2.0 / std::numbers::pi_v<double>));
    const Series c1 = a.div(b).mul(2.0 / std::numbers::pi_v<double>);
    const Series cv_int = cp_over_r.sub(2.5).sub(crot_);
    const Series f_rot = f_int.mul(c1.add(1.0));
    const Series f_trans = c1.mul(crot_).div(1.5).neg().add(1.0).mul(2.5);
    const Series lambda = eta.div(mass_).mul(r).mul(
        f_trans.mul(1.5).add(crot_.mul(f_rot)).add(cv_int.mul(f_int)));
    return {eta, lambda, binary_diffusion(t)};
  }

  // Wilke mixture viscosity. x [..., S].
  [[nodiscard]] Series mixture_viscosity(const Species &sp,
                                         const Series &x) const {
    const Series xx = x.clamp(1e-20, 1.0);
    const Series ratio = sp.viscosity.unsqueeze(-1)
                             .div(sp.viscosity.unsqueeze(-2))
                             .sqrt(); // sqrt(eta_k/eta_j)
    const Series phi =
        ratio.mul(wilke_mass_).add(1.0).square().mul(wilke_inv_); // [..., k, j]
    const Series denom =
        phi.matmul(xx.unsqueeze(-1)).squeeze(-1); // sum_j x_j phi_kj
    return xx.mul(sp.viscosity).div(denom).sum(-1, false);
  }
  // Arithmetic-harmonic conductivity average.
  [[nodiscard]] Series mixture_conductivity(const Species &sp,
                                            const Series &x) const {
    const Series xx = x.clamp(1e-20, 1.0);
    const Series arith = xx.mul(sp.conductivity).sum(-1, false);
    const Series harm = xx.div(sp.conductivity).sum(-1, false);
    return arith.add(harm.reciprocal()).mul(0.5);
  }
  // Mixture-averaged diffusion coefficients D_k [..., S] at pressure p [...]
  // (mole-fraction based).
  [[nodiscard]] Series mixture_diffusion(const Species &sp, const Series &x,
                                         const Series &pressure) const {
    const Series xx = x.clamp(1e-20, 1.0);
    const Series inv = sp.binary_diffusion.reciprocal().mul(
        eye_.neg().add(1.0)); // 1/D_jk, zero diagonal
    const Series sum =
        inv.matmul(xx.unsqueeze(-1)).squeeze(-1); // sum_{j != k} x_j / D_kj
    return xx.neg().add(1.0).div(sum).div(pressure.unsqueeze(-1));
  }
  // CHEMKIN D_km relating the mass-averaged diffusive flux to mole-fraction
  // gradients: D_k = (1 - Y_k) / (p sum_{j != k} x_j / D_jk) = (M_mix - x_k
  // M_k) / (p M_mix sum_{j != k} x_j / D_jk). (Cantera's default
  // `mix_diff_coeffs`; the mole form above is its `mix_diff_coeffs_mole`.)
  [[nodiscard]] Series mixture_diffusion_chemkin(const Species &sp,
                                                 const Series &x,
                                                 const Series &pressure) const {
    const Series xx = x.clamp(1e-20, 1.0);
    const Series m_mix = xx.mul(mass_).sum(-1, true);
    const Series inv =
        sp.binary_diffusion.reciprocal().mul(eye_.neg().add(1.0));
    const Series sum = inv.matmul(xx.unsqueeze(-1)).squeeze(-1);
    return m_mix.sub(xx.mul(mass_))
        .div(m_mix.mul(sum))
        .div(pressure.unsqueeze(-1));
  }

  [[nodiscard]] Series mixture_diffusion_chemkin(const Series &temperature,
                                                 const Series &x,
                                                 const Series &pressure) const {
    Species diffusion_only;
    diffusion_only.binary_diffusion = binary_diffusion(temperature);
    return mixture_diffusion_chemkin(diffusion_only, x, pressure);
  }

private:
  std::vector<std::string> names_;
  Series eps_ = pluss::series::undefined(), sigma_ = pluss::series::undefined(),
         mass_ = pluss::series::undefined(), crot_ = pluss::series::undefined(),
         zrot_ = pluss::series::undefined();
  Series reduced_mass_ = pluss::series::undefined(),
         sigma_pair_ = pluss::series::undefined(),
         eps_pair_ = pluss::series::undefined(),
         wilke_mass_ = pluss::series::undefined(),
         wilke_inv_ = pluss::series::undefined(),
         eye_ = pluss::series::undefined();
};

} // namespace pluss::subsp::natsci::chemistry::transport
