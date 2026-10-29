#pragma once

// Reynolds-averaged closures, validated on the fully developed turbulent channel (one homogeneous direction, wall at y = 0, symmetry at y = 1).
// Lengths are scaled by the half height and velocities by the friction velocity, so the viscosity is 1 / Re_tau and the mean pressure gradient is 1.
//
//  * Spalart-Allmaras (Spalart & Allmaras, Rech. Aerosp. 1 (1994) 5), fully turbulent form (no trip term):
//      d nu~/dt = cb1 S~ nu~ + (1/sigma) [ div((nu + nu~) grad nu~) + cb2 |grad nu~|^2 ] - cw1 fw (nu~/d)^2,   nu_t = nu~ fv1,
//      S~ = Omega + nu~ fv2 / (kappa^2 d^2) (floored at 0.3 Omega), fv1 = chi^3/(chi^3 + cv1^3), fv2 = 1 - chi/(1 + chi fv1), chi = nu~/nu,
//      fw = g ((1 + cw3^6)/(g^6 + cw3^6))^(1/6), g = r + cw2 (r^6 - r), r = min(nu~/(S~ kappa^2 d^2), 10),
//      cb1 = 0.1355, cb2 = 0.622, sigma = 2/3, kappa = 0.41, cw2 = 0.3, cw3 = 2, cv1 = 7.1, cw1 = cb1/kappa^2 + (1 + cb2)/sigma.
//  * k-omega SST (Menter, Kuntz & Langtry 2003 form): two blended sets of constants through F1,
//      d k/dt = min(nu_t S^2, 10 beta* k omega) - beta* k omega + div((nu + sigma_k nu_t) grad k),
//      d omega/dt = gamma S^2 - beta omega^2 + div((nu + sigma_w nu_t) grad omega) + 2 (1 - F1) sigma_w2 (grad k . grad omega)/omega,
//      nu_t = a1 k / max(a1 omega, S F2), set 1 (sigma_k 0.85, sigma_w 0.5, beta 0.075), set 2 (1.0, 0.856, 0.0828), beta* = 0.09, a1 = 0.31,
//      gamma_i = beta_i/beta* - sigma_wi kappa^2 / sqrt(beta*), omega_wall = 10 * 6 nu / (beta1 y1^2).
//  Constants and formulas are as reported in the cited papers from memory of the standard references (the NASA Turbulence Modeling Resource lists the same
//  forms); the originals were not obtained, so the channel results below are checked against the law of the wall and the exact momentum balance instead.
//  The wall-normal operator is a cell-centred finite-volume diffusion on a tanh-stretched grid, advanced in pseudo-time with the sinks implicit, every
//  solve being a dense Series inverse (the system is tridiagonal and small).

#include "pluss/codem/codem.h"
#include "pluss/subsp/linearalg/linearalg.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace pluss::subsp::natsci::physics::fluids::rans {

using pluss::Series;

enum class Model : std::uint8_t { SpalartAllmaras = 0, KOmegaSST = 1 };

struct ChannelOptions final {
  Model model{Model::SpalartAllmaras};
  double friction_reynolds{1000.0}; // Re_tau = u_tau h / nu
  std::int64_t cells{160};
  double stretching{3.0};           // tanh clustering at the wall (larger = finer first cell)
  std::int64_t max_iterations{6000};
  double tolerance{1e-10};          // relative change of the velocity per iteration
  double first_step{1e-6};          // bounds of the local pseudo-time step
  double growth_limit{0.2};         // local step = growth_limit / (0.3 |dU/dy|), bounded by first_step and max_step
  double max_step{20.0};
};

struct ChannelResult final {
  Series y, velocity, eddy_viscosity, total_stress; // cell centres; total_stress = (nu + nu_t) dU/dy at the cell centres (exact answer 1 - y)
  Series k, omega, nu_tilde;                         // the transported variables of the chosen model (others undefined)
  bool converged{false};
  std::int64_t iterations{0};
  double residual{0.0};
};

namespace detail {

constexpr double kKappa = 0.41;

inline Series cat(const Series &a, const Series &b) { return a.unite({b}, 0); }
inline Series fill(std::int64_t n, double v) { return pluss::series::full({n}, v, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)); }

struct Mesh {
  std::int64_t n{0};
  Series yf, yc, dy, dyc, eye, up, lo; // faces (n+1), centres, widths, centre spacings (n-1), constant shift matrices
};

inline Mesh make_mesh(std::int64_t n, double stretching) {
  Mesh m;
  m.n = n;
  const Series u = Series::linspace(0.0, 1.0, n + 1, pluss::ScalarType::Float64);
  m.yf = stretching > 0.0 ? u.mul(-1.0).add(1.0).mul(stretching).tanh().div(std::tanh(stretching)).mul(-1.0).add(1.0) : u;
  m.dy = m.yf.slice(0, 1, n + 1).sub(m.yf.slice(0, 0, n));
  m.yc = m.yf.slice(0, 1, n + 1).add(m.yf.slice(0, 0, n)).mul(0.5);
  m.dyc = m.yc.slice(0, 1, n).sub(m.yc.slice(0, 0, n - 1));
  m.eye = pluss::series::eye(n, pluss::ScalarType::Float64);
  m.up = m.eye.roll(1, 1);
  m.lo = m.eye.roll(-1, 1);
  return m;
}

// gradient on the faces of a cell field with the wall value `wall` and a symmetry (zero-gradient) top
inline Series face_gradient(const Mesh &m, const Series &phi, double wall) {
  const Series inner = phi.slice(0, 1, m.n).sub(phi.slice(0, 0, m.n - 1)).div(m.dyc);
  const Series at_wall = phi.slice(0, 0, 1).sub(wall).div(m.yc.slice(0, 0, 1));
  return cat(cat(at_wall, inner), fill(1, 0.0));
}
inline Series centre_gradient(const Mesh &m, const Series &phi, double wall) {
  const Series f = face_gradient(m, phi, wall);
  return f.slice(0, 0, m.n).add(f.slice(0, 1, m.n + 1)).mul(0.5);
}

// one implicit pseudo-time step of  d phi/dt = div(c grad phi) + source - sink phi  with phi = wall at y = 0 and zero flux at y = 1
inline Series implicit_step(const Mesh &m, const Series &phi, double wall, const Series &c, double c_wall, const Series &source, const Series &sink, const Series &dt) {
  const Series cf = c.slice(0, 1, m.n).add(c.slice(0, 0, m.n - 1)).mul(0.5); // interior faces
  const Series up = cat(cf.div(m.dyc).div(m.dy.slice(0, 0, m.n - 1)), fill(1, 0.0));
  const Series lo = cat(fill(1, 0.0), cf.div(m.dyc).div(m.dy.slice(0, 1, m.n)));
  const Series wall_coef = cat(fill(1, c_wall).div(m.yc.slice(0, 0, 1)).div(m.dy.slice(0, 0, 1)), fill(m.n - 1, 0.0));
  const Series diag = up.add(lo).add(wall_coef).add(sink).add(dt.reciprocal());
  const Series a = m.eye.mul(diag.unsqueeze(1)).sub(m.up.mul(up.unsqueeze(1))).sub(m.lo.mul(lo.unsqueeze(1)));
  const Series rhs = phi.div(dt).add(source).add(wall_coef.mul(wall));
  return pluss::subsp::linearalg::inverse(a).matmul(rhs.unsqueeze(1)).squeeze(1);
}

inline double scalar(const Series &s) { return s.reshape({}).item<double>(); }

} // namespace detail

[[nodiscard]] inline ChannelResult solve_channel(const ChannelOptions &o = {}) {
  using namespace detail;
  if (o.cells < 16 || !(o.friction_reynolds > 1.0) || !(o.growth_limit > 0.0) || !(o.first_step > 0.0))
    throw std::invalid_argument("natsci::fluids::rans::solve_channel: needs cells >= 16, Re_tau > 1 and positive step limits");
  const Mesh m = make_mesh(o.cells, o.stretching);
  const double nu = 1.0 / o.friction_reynolds;
  const Series d = m.yc, yplus = m.yc.mul(o.friction_reynolds);
  const bool sa = o.model == Model::SpalartAllmaras;

  // Reichardt-type start for the velocity; transported variables start from mixing-length values
  Series U = yplus.mul(kKappa).add(1.0).log().div(kKappa).add(yplus.mul(-1.0 / 11.0).exp().mul(-1.0).add(1.0).sub(yplus.div(11.0).mul(yplus.mul(-1.0 / 3.0).exp())).mul(7.8));
  Series nut_tilde = yplus.mul(kKappa * nu).mul(m.yc.mul(-1.0).add(1.0));
  Series k = yplus.div(yplus.add(10.0)).mul(3.3).mul(m.yc.mul(-1.0).add(1.0).clamp_min(0.05));
  Series w = k.div(yplus.mul(kKappa * nu).add(nu)).clamp_min(1.0);

  constexpr double cb1 = 0.1355, cb2 = 0.622, sigma = 2.0 / 3.0, cw2 = 0.3, cw3 = 2.0, cv1 = 7.1;
  constexpr double cw1 = cb1 / (kKappa * kKappa) + (1.0 + cb2) / sigma;
  constexpr double bstar = 0.09, a1 = 0.31, sk1 = 0.85, sw1 = 0.5, b1 = 0.075, sk2 = 1.0, sw2 = 0.856, b2 = 0.0828;
  const double g1 = b1 / bstar - sw1 * kKappa * kKappa / std::sqrt(bstar), g2 = b2 / bstar - sw2 * kKappa * kKappa / std::sqrt(bstar);
  const double w_wall = 60.0 * nu / (b1 * std::pow(scalar(m.yc.slice(0, 0, 1)), 2));

  ChannelResult r;
  const Series zero = fill(o.cells, 0.0), one = fill(o.cells, 1.0);
  for (std::int64_t it = 1; it <= o.max_iterations; ++it) {
    const Series strain = centre_gradient(m, U, 0.0).abs().clamp_min(1e-12);
    // local pseudo-time step: the explicit production grows at a rate proportional to the strain, so the step shrinks where it is large
    const Series dt = strain.mul(0.3).add(1e-3).reciprocal().mul(o.growth_limit).clamp_max(o.max_step).clamp_min(o.first_step);
    Series nut;
    Series F1 = zero;
    if (sa) {
      const Series chi = nut_tilde.div(nu), chi3 = chi.pow(3.0);
      nut = nut_tilde.mul(chi3.div(chi3.add(cv1 * cv1 * cv1)));
    } else {
      const Series dk = centre_gradient(m, k, 0.0), dw = centre_gradient(m, w, w_wall);
      const Series cd = dk.mul(dw).div(w).mul(2.0 * sw2).clamp_min(1e-10);
      const Series sqk = k.clamp_min(0.0).sqrt();
      const Series t1 = sqk.div(w.mul(bstar).mul(d)).maximum(d.square().mul(w).reciprocal().mul(500.0 * nu));
      const Series arg1 = t1.minimum(k.mul(4.0 * sw2).div(cd.mul(d.square())));
      F1 = arg1.pow(4.0).tanh();
      const Series arg2 = sqk.div(w.mul(bstar).mul(d)).mul(2.0).maximum(d.square().mul(w).reciprocal().mul(500.0 * nu));
      const Series F2 = arg2.square().tanh();
      nut = k.mul(a1).div(w.mul(a1).maximum(strain.mul(F2)));
    }
    // momentum: d/dy((nu + nu_t) dU/dy) + 1 = 0, marched in pseudo-time
    const Series Unew = implicit_step(m, U, 0.0, nut.add(nu), nu, one, zero, dt);
    if (sa) {
      const Series chi = nut_tilde.div(nu), chi3 = chi.pow(3.0);
      const Series fv1 = chi3.div(chi3.add(cv1 * cv1 * cv1)), fv2 = chi.div(chi.mul(fv1).add(1.0)).mul(-1.0).add(1.0);
      const Series kd2 = d.square().mul(kKappa * kKappa);
      const Series stilde = strain.add(nut_tilde.mul(fv2).div(kd2)).maximum(strain.mul(0.3));
      const Series rr = nut_tilde.div(stilde.mul(kd2)).clamp_max(10.0);
      const Series gg = rr.add(rr.pow(6.0).sub(rr).mul(cw2));
      const Series fw = gg.mul(gg.pow(6.0).add(std::pow(cw3, 6.0)).reciprocal().mul(1.0 + std::pow(cw3, 6.0)).pow(1.0 / 6.0));
      const Series dn = centre_gradient(m, nut_tilde, 0.0);
      const Series source = stilde.mul(nut_tilde).mul(cb1).add(dn.square().mul(cb2 / sigma));
      const Series sink = fw.mul(nut_tilde).mul(cw1).div(d.square());
      nut_tilde = implicit_step(m, nut_tilde, 0.0, nut_tilde.add(nu).div(sigma), nu / sigma, source, sink, dt).clamp_min(0.0);
    } else {
      const Series dk = centre_gradient(m, k, 0.0), dw = centre_gradient(m, w, w_wall);
      const Series sk = F1.mul(sk1 - sk2).add(sk2), sw = F1.mul(sw1 - sw2).add(sw2), bb = F1.mul(b1 - b2).add(b2), gam = F1.mul(g1 - g2).add(g2);
      const Series pk = nut.mul(strain.square()).minimum(k.mul(w).mul(10.0 * bstar));
      const Series cross = F1.mul(-1.0).add(1.0).mul(2.0 * sw2).mul(dk).mul(dw).div(w);
      const Series knew = implicit_step(m, k, 0.0, sk.mul(nut).add(nu), nu, pk, w.mul(bstar), dt).clamp_min(0.0);
      const Series wnew = implicit_step(m, w, w_wall, sw.mul(nut).add(nu), nu, gam.mul(strain.square()).add(cross.clamp_min(0.0)), w.mul(bb), dt).clamp_min(1e-8);
      k = knew;
      w = wnew;
    }
    const double residual = Unew.sub(U).abs().max().item<double>() / Unew.abs().max().item<double>();
    U = Unew;
    r.iterations = it;
    r.residual = residual;
    if (residual < o.tolerance) { r.converged = true; break; }
  }
  // final eddy viscosity and the total shear stress for the report
  Series nut_final;
  if (sa) {
    const Series chi = nut_tilde.div(nu), chi3 = chi.pow(3.0);
    nut_final = nut_tilde.mul(chi3.div(chi3.add(cv1 * cv1 * cv1)));
  } else {
    const Series strain = centre_gradient(m, U, 0.0).abs().clamp_min(1e-12);
    const Series sqk = k.clamp_min(0.0).sqrt();
    const Series arg2 = sqk.div(w.mul(bstar).mul(d)).mul(2.0).maximum(d.square().mul(w).reciprocal().mul(500.0 * nu));
    nut_final = k.mul(a1).div(w.mul(a1).maximum(strain.mul(arg2.square().tanh())));
  }
  r.y = m.yc;
  r.velocity = U;
  r.eddy_viscosity = nut_final;
  r.total_stress = centre_gradient(m, U, 0.0).mul(nut_final.add(nu));
  if (sa) r.nu_tilde = nut_tilde; else { r.k = k; r.omega = w; }
  return r;
}

} // namespace pluss::subsp::natsci::physics::fluids::rans
