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
#include "pluss/subsp/natsci/physics/fluids/les.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

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


// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Coupling to the incompressible stepper. The model variables live on the cell-centred grid next to the pressure; every step first advances them with the
// resolved velocity (upwind advection, conservative variable-coefficient diffusion, production from |S| = sqrt(2 S_ij S_ij), destruction implicit and the
// diffusion sub-cycled to its stability limit), then forms nu_t from the updated variables and advances the momentum equation with the total viscosity
// nu + nu_t frozen over the Runge-Kutta stages (the transport is operator-split from the momentum step, first order in time). No-slip faces carry the wall
// values (nu~ = k = 0, omega = 60 nu / (beta1 (h/2)^2)); every other face is zero-gradient. The wall distance is the distance to the nearest no-slip face
// (a periodic grid has none, so a prescribed field can be given instead).
// ---------------------------------------------------------------------------------------------------------------------------------------------------

struct Transport final {
  Series nu_tilde = pluss::series::undefined(); // Spalart-Allmaras
  Series k = pluss::series::undefined();        // SST turbulent kinetic energy
  Series omega = pluss::series::undefined();    // SST specific dissipation
};

struct Closure final {
  Model model{Model::SpalartAllmaras};
  double freestream{3.0}; // initial nu~ / nu (SA) and nu_t / nu (SST) away from walls
};

struct CoupledStep final {
  fluids::StepResult flow;
  Transport transport;
};

namespace detail {

namespace fl = ::pluss::subsp::natsci::physics::fluids;
namespace sp = ::pluss::subsp::natsci::common::spatial;

// shape [1, .., n, .., 1] with value 1 at `index` along `axis` of a `dim`-dimensional field
inline Series edge_flag(std::int64_t n, std::int64_t index, std::int64_t axis, std::int64_t dim) {
  std::vector<double> v(static_cast<std::size_t>(n), 0.0);
  v[static_cast<std::size_t>(index)] = 1.0;
  std::vector<std::int64_t> shape(static_cast<std::size_t>(dim), 1);
  shape[static_cast<std::size_t>(axis)] = n;
  return pluss::make_series(v, shape);
}

// What the transport update needs from a grid: spacings, neighbours with the wall ghosts, the strain and the wall distance.
struct Domain final {
  std::int64_t dim{0};
  std::vector<std::int64_t> shape;
  std::vector<double> spacing;
  std::vector<bool> wall_low, wall_high;  // no-slip faces
  bool periodic{false};
  Series distance = pluss::series::undefined();
  // neighbours of phi along `axis`; at a no-slip face the ghost is 2 wall - phi (Dirichlet), elsewhere phi itself (zero gradient) or the periodic wrap
  std::pair<Series, Series> neighbors(const Series &phi, std::int64_t axis, double wall, bool dirichlet = true) const {
    if (periodic) return {phi.roll(1, axis), phi.roll(-1, axis)};
    const std::int64_t n = shape[static_cast<std::size_t>(axis)];
    Series low = phi.slice(axis, 0, 1).unite({phi.slice(axis, 0, n - 1)}, axis);
    Series high = phi.slice(axis, 1, n).unite({phi.slice(axis, n - 1, n)}, axis);
    if (dirichlet && wall_low[static_cast<std::size_t>(axis)]) low = low.add(edge_flag(n, 0, axis, dim).mul(phi.sub(wall).mul(-2.0)));
    if (dirichlet && wall_high[static_cast<std::size_t>(axis)]) high = high.add(edge_flag(n, n - 1, axis, dim).mul(phi.sub(wall).mul(-2.0)));
    return {low, high};
  }
};

// wall distance to the nearest no-slip face of a box grid
inline Series box_distance(const std::vector<std::int64_t> &shape, const std::vector<double> &h, const std::vector<bool> &lo, const std::vector<bool> &hi) {
  const std::int64_t dim = static_cast<std::int64_t>(shape.size());
  Series d = pluss::series::full(shape, 1e30, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  for (std::int64_t a = 0; a < dim; ++a) {
    const std::int64_t n = shape[static_cast<std::size_t>(a)];
    std::vector<std::int64_t> bshape(static_cast<std::size_t>(dim), 1);
    bshape[static_cast<std::size_t>(a)] = n;
    std::vector<double> to_low(static_cast<std::size_t>(n)), to_high(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) {
      to_low[static_cast<std::size_t>(i)] = (static_cast<double>(i) + 0.5) * h[static_cast<std::size_t>(a)];
      to_high[static_cast<std::size_t>(i)] = (static_cast<double>(n - i) - 0.5) * h[static_cast<std::size_t>(a)];
    }
    if (lo[static_cast<std::size_t>(a)]) d = d.minimum(pluss::make_series(to_low, bshape));
    if (hi[static_cast<std::size_t>(a)]) d = d.minimum(pluss::make_series(to_high, bshape));
  }
  return d;
}

inline Series sa_nut(const Series &nt, double nu) {
  const Series chi3 = nt.div(nu).pow(3.0);
  return nt.mul(chi3.div(chi3.add(7.1 * 7.1 * 7.1)));
}

inline Series sst_nut(const Series &k, const Series &w, const Series &s, const Series &d, double nu) {
  const Series sqk = k.clamp_min(0.0).sqrt();
  const Series arg2 = sqk.div(w.mul(0.09).mul(d)).mul(2.0).maximum(d.square().mul(w).reciprocal().mul(500.0 * nu));
  return k.mul(0.31).div(w.mul(0.31).maximum(s.mul(arg2.square().tanh())));
}

// d phi/dt = div(c grad phi) - u . grad phi + source - sink phi over `dt`, sub-cycled for the explicit diffusion and advection limits
inline Series transport_update(const Domain &dm, const Series &phi, double wall, const Series &c, const Series &source, const Series &sink, const Series &velocity,
                               std::int64_t velocity_axis, double dt) {
  double cmax = c.max().item<double>(), umax = velocity.abs().max().item<double>(), hmin = *std::min_element(dm.spacing.begin(), dm.spacing.end());
  const double limit = 0.4 * hmin * hmin / (2.0 * static_cast<double>(dm.dim) * cmax + hmin * umax + 1e-300);
  const std::int64_t sub = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::ceil(dt / limit)));
  const double h = dt / static_cast<double>(sub);
  Series x = phi;
  for (std::int64_t s = 0; s < sub; ++s) {
    Series rate = source;
    for (std::int64_t a = 0; a < dm.dim; ++a) {
      const double ha = dm.spacing[static_cast<std::size_t>(a)];
      const auto [lo, hi] = dm.neighbors(x, a, wall);
      const auto [clo, chi] = dm.neighbors(c, a, 0.0, false);
      const Series flux_hi = c.add(chi).mul(0.5).mul(hi.sub(x)), flux_lo = c.add(clo).mul(0.5).mul(x.sub(lo));
      rate = rate.add(flux_hi.sub(flux_lo).div(ha * ha));
      const Series u = velocity.select(velocity_axis, a);
      rate = rate.sub(u.clamp_min(0.0).mul(x.sub(lo)).add(u.clamp_max(0.0).mul(hi.sub(x))).div(ha));
    }
    x = x.add(rate.mul(h)).div(sink.mul(h).add(1.0)).clamp_min(0.0);
  }
  return x;
}

inline Series gradient_square(const Domain &dm, const Series &phi, double wall) {
  Series sum = pluss::series::zeros_like(phi);
  for (std::int64_t a = 0; a < dm.dim; ++a) {
    const auto [lo, hi] = dm.neighbors(phi, a, wall);
    sum = sum.add(hi.sub(lo).div(2.0 * dm.spacing[static_cast<std::size_t>(a)]).square());
  }
  return sum;
}
inline Series gradient_dot(const Domain &dm, const Series &p, double wp, const Series &q, double wq) {
  Series sum = pluss::series::zeros_like(p);
  for (std::int64_t a = 0; a < dm.dim; ++a) {
    const double h = 2.0 * dm.spacing[static_cast<std::size_t>(a)];
    const auto [pl, ph] = dm.neighbors(p, a, wp);
    const auto [ql, qh] = dm.neighbors(q, a, wq);
    sum = sum.add(ph.sub(pl).div(h).mul(qh.sub(ql).div(h)));
  }
  return sum;
}

inline Transport update_transport(const Domain &dm, const Closure &cl, const Transport &t, const Series &velocity, std::int64_t velocity_axis, const Series &strain,
                                  double nu, double dt) {
  using namespace ::pluss::subsp::natsci::physics::fluids::rans::detail;
  const Series d = dm.distance.clamp_min(1e-12);
  Transport out;
  if (cl.model == Model::SpalartAllmaras) {
    constexpr double cb1 = 0.1355, cb2 = 0.622, sigma = 2.0 / 3.0, cw2 = 0.3, cw3 = 2.0, cv1 = 7.1;
    constexpr double cw1 = cb1 / (kKappa * kKappa) + (1.0 + cb2) / sigma;
    const Series chi = t.nu_tilde.div(nu), chi3 = chi.pow(3.0);
    const Series fv1 = chi3.div(chi3.add(cv1 * cv1 * cv1)), fv2 = chi.div(chi.mul(fv1).add(1.0)).mul(-1.0).add(1.0);
    const Series kd2 = d.square().mul(kKappa * kKappa);
    const Series stilde = strain.add(t.nu_tilde.mul(fv2).div(kd2)).maximum(strain.mul(0.3));
    const Series rr = t.nu_tilde.div(stilde.mul(kd2).clamp_min(1e-300)).clamp_max(10.0);
    const Series gg = rr.add(rr.pow(6.0).sub(rr).mul(cw2));
    const Series fw = gg.mul(gg.pow(6.0).add(std::pow(cw3, 6.0)).reciprocal().mul(1.0 + std::pow(cw3, 6.0)).pow(1.0 / 6.0));
    const Series source = stilde.mul(t.nu_tilde).mul(cb1).add(gradient_square(dm, t.nu_tilde, 0.0).mul(cb2 / sigma));
    const Series sink = fw.mul(t.nu_tilde).mul(cw1).div(d.square());
    out.nu_tilde = transport_update(dm, t.nu_tilde, 0.0, t.nu_tilde.add(nu).div(sigma), source, sink, velocity, velocity_axis, dt);
    return out;
  }
  constexpr double bstar = 0.09, a1 = 0.31, sk1 = 0.85, sw1 = 0.5, b1 = 0.075, sk2 = 1.0, sw2 = 0.856, b2 = 0.0828;
  const double g1 = b1 / bstar - sw1 * kKappa * kKappa / std::sqrt(bstar), g2 = b2 / bstar - sw2 * kKappa * kKappa / std::sqrt(bstar);
  double hwall = 0.0;
  for (std::int64_t a = 0; a < dm.dim; ++a)
    if (dm.wall_low[static_cast<std::size_t>(a)] || dm.wall_high[static_cast<std::size_t>(a)])
      hwall = hwall == 0.0 ? dm.spacing[static_cast<std::size_t>(a)] : std::min(hwall, dm.spacing[static_cast<std::size_t>(a)]);
  const double w_wall = hwall > 0.0 ? 60.0 * nu / (b1 * std::pow(0.5 * hwall, 2)) : 0.0;
  const Series nut = sst_nut(t.k, t.omega, strain, d, nu);
  const Series dkdw = gradient_dot(dm, t.k, 0.0, t.omega, w_wall);
  const Series cd = dkdw.div(t.omega).mul(2.0 * sw2).clamp_min(1e-10);
  const Series sqk = t.k.clamp_min(0.0).sqrt();
  const Series t1 = sqk.div(t.omega.mul(bstar).mul(d)).maximum(d.square().mul(t.omega).reciprocal().mul(500.0 * nu));
  const Series F1 = t1.minimum(t.k.mul(4.0 * sw2).div(cd.mul(d.square()))).pow(4.0).tanh();
  const Series sk = F1.mul(sk1 - sk2).add(sk2), sw = F1.mul(sw1 - sw2).add(sw2), bb = F1.mul(b1 - b2).add(b2), gam = F1.mul(g1 - g2).add(g2);
  const Series pk = nut.mul(strain.square()).minimum(t.k.mul(t.omega).mul(10.0 * bstar));
  const Series cross = F1.mul(-1.0).add(1.0).mul(2.0 * sw2).mul(dkdw).div(t.omega).clamp_min(0.0);
  out.k = transport_update(dm, t.k, 0.0, sk.mul(nut).add(nu), pk, t.omega.mul(bstar), velocity, velocity_axis, dt);
  out.omega = transport_update(dm, t.omega, w_wall, sw.mul(nut).add(nu), gam.mul(strain.square()).add(cross), t.omega.mul(bb), velocity, velocity_axis, dt).clamp_min(1e-8);
  return out;
}

inline Domain periodic_domain(const sp::PeriodicGrid &grid, const Series &distance) {
  Domain dm;
  dm.dim = grid.dim();
  dm.periodic = true;
  for (std::int64_t a = 0; a < dm.dim; ++a) {
    dm.shape.push_back(grid.shape()[static_cast<std::size_t>(a)]);
    dm.spacing.push_back(grid.spacing(a).si_value());
  }
  dm.wall_low.assign(static_cast<std::size_t>(dm.dim), false);
  dm.wall_high.assign(static_cast<std::size_t>(dm.dim), false);
  dm.distance = distance.defined() ? distance : pluss::series::full(dm.shape, 1e30, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  return dm;
}

inline Domain box_domain(const sp::BoxGrid &grid, const fl::FlowBounds &bounds) {
  Domain dm;
  dm.dim = grid.dim();
  for (std::int64_t a = 0; a < dm.dim; ++a) {
    dm.shape.push_back(grid.shape()[static_cast<std::size_t>(a)]);
    dm.spacing.push_back(grid.spacing(a).si_value());
    dm.wall_low.push_back(bounds.at(a, sp::Side::Low).kind() == fl::FlowKind::NoSlip);
    dm.wall_high.push_back(bounds.at(a, sp::Side::High).kind() == fl::FlowKind::NoSlip);
  }
  dm.distance = box_distance(dm.shape, dm.spacing, dm.wall_low, dm.wall_high);
  return dm;
}

inline Series eddy_from(const Domain &dm, const Closure &cl, const Transport &t, const Series &strain, double nu) {
  return cl.model == Model::SpalartAllmaras ? sa_nut(t.nu_tilde, nu) : sst_nut(t.k, t.omega, strain, dm.distance.clamp_min(1e-12), nu);
}

} // namespace detail

// uniform free-stream values of the model variables on the grid of `state`
[[nodiscard]] inline Transport initial_transport(const Closure &closure, const common::ScalarQuantity<fluids::KinematicViscosity> &viscosity,
                                                 const fluids::IncompressibleState &state, double omega = 10.0) {
  const Series ones = pluss::series::zeros_like(state.pressure().si_value()).add(1.0);
  const double nu = viscosity.si_value();
  Transport t;
  if (closure.model == Model::SpalartAllmaras) {
    t.nu_tilde = ones.mul(closure.freestream * nu);
  } else {
    t.omega = ones.mul(omega);
    t.k = ones.mul(closure.freestream * nu * omega);
  }
  return t;
}

// eddy viscosity [m^2/s] from the model variables and the resolved strain
[[nodiscard]] inline common::SeriesQuantity<fluids::KinematicViscosity> eddy_viscosity(const fluids::IncompressibleState &state, const Transport &transport,
                                                                                    const spatial::BoxGrid &grid, const fluids::FlowBounds &bounds,
                                                                                    const common::ScalarQuantity<fluids::KinematicViscosity> &viscosity, const Closure &closure = {}) {
  const detail::Domain dm = detail::box_domain(grid, bounds);
  const std::int64_t ca = state.velocity().si_value().dim() - grid.dim() - 1;
  const Series s = fluids::les::detail::strain_norm(fluids::les::detail::components(fluids::detail::vel_grad(state.velocity(), grid, bounds), ca, grid.dim()));
  return common::SeriesQuantity<fluids::KinematicViscosity>::from_si(detail::eddy_from(dm, closure, transport, s, viscosity.si_value()));
}

[[nodiscard]] inline CoupledStep advance(const fluids::IncompressibleState &state, const Transport &transport, const spatial::BoxGrid &grid, const fluids::FlowBounds &bounds,
                                         const common::ScalarQuantity<fluids::Density> &density, const common::ScalarQuantity<fluids::KinematicViscosity> &viscosity,
                                         const Closure &closure, common::ScalarQuantity<common::Time> time, common::ScalarQuantity<common::Time> step,
                                         const fluids::StepOptions &options = {}, const fluids::BodyFn &body = {}) {
  if (options.rule == fluids::StepRule::Imex1)
    throw std::invalid_argument("natsci::fluids::rans::advance: Imex1 needs scalar viscosity; use an explicit rule with a closure");
  const detail::Domain dm = detail::box_domain(grid, bounds);
  const double nu = viscosity.si_value();
  const Series &velocity = state.velocity().si_value();
  const std::int64_t ca = velocity.dim() - grid.dim() - 1;
  const Series strain = fluids::les::detail::strain_norm(fluids::les::detail::components(fluids::detail::vel_grad(state.velocity(), grid, bounds), ca, grid.dim()));
  const Transport next = detail::update_transport(dm, closure, transport, velocity, ca, strain, nu, step.si_value());
  const Series nut = detail::eddy_from(dm, closure, next, strain, nu);
  const Series ones = pluss::series::zeros_like(state.pressure().si_value()).add(1.0);
  const auto rho = common::SeriesQuantity<fluids::Density>::from_si(ones.mul(density.si_value()));
  const auto total = common::SeriesQuantity<fluids::DynamicViscosity>::from_si(nut.add(nu).mul(density.si_value()));
  fluids::StepLimits limits = fluids::detail::prepare_step(state, grid, rho, total, time, step, options);
  const auto project_stage = [&](const fluids::IncompressibleState &candidate) {
    return fluids::detail::project_core(candidate, grid, bounds, density, step,
                                        common::SeriesQuantity<common::Frequency>::from_si(fluids::detail::vel_divergence(candidate.velocity(), grid, bounds)),
                                        options.projection.poisson)
        .state;
  };
  const auto rate = [&](const fluids::IncompressibleState &at, common::ScalarQuantity<common::Time> when) {
    return fluids::detail::rate_at(at, grid, bounds, when, rho, total, body, options.form);
  };
  const auto finalize = [&](const fluids::IncompressibleState &candidate) {
    auto checked = fluids::make_state(candidate.velocity(), candidate.pressure(), grid);
    return std::pair{checked, fluids::div_report(checked.velocity(), grid, bounds)};
  };
  return CoupledStep{fluids::detail::advance_stages(state, grid, time, step, std::move(limits), options, project_stage, rate, finalize), next};
}

[[nodiscard]] inline CoupledStep advance(const fluids::IncompressibleState &state, const Transport &transport, const spatial::PeriodicGrid &grid,
                                         const common::ScalarQuantity<fluids::Density> &density, const common::ScalarQuantity<fluids::KinematicViscosity> &viscosity,
                                         const Closure &closure, common::ScalarQuantity<common::Time> time, common::ScalarQuantity<common::Time> step,
                                         const fluids::StepOptions &options = {}, const fluids::BodyFn &body = {}, const Series &wall_distance = pluss::series::undefined()) {
  if (options.rule == fluids::StepRule::Imex1)
    throw std::invalid_argument("natsci::fluids::rans::advance: Imex1 needs scalar viscosity; use an explicit rule with a closure");
  const detail::Domain dm = detail::periodic_domain(grid, wall_distance);
  const double nu = viscosity.si_value();
  const Series &velocity = state.velocity().si_value();
  const std::int64_t ca = velocity.dim() - grid.dim() - 1;
  const Series strain = fluids::les::detail::strain_norm(fluids::les::detail::components(spatial::gradient(velocity, grid), ca, grid.dim()));
  const Transport next = detail::update_transport(dm, closure, transport, velocity, ca, strain, nu, step.si_value());
  const Series nut = detail::eddy_from(dm, closure, next, strain, nu);
  const Series ones = pluss::series::zeros_like(state.pressure().si_value()).add(1.0);
  const auto rho = common::SeriesQuantity<fluids::Density>::from_si(ones.mul(density.si_value()));
  const auto total = common::SeriesQuantity<fluids::DynamicViscosity>::from_si(nut.add(nu).mul(density.si_value()));
  fluids::StepLimits limits = fluids::detail::prepare_step(state, grid, rho, total, time, step, options);
  const auto project_stage = [&](const fluids::IncompressibleState &candidate) {
    return fluids::detail::project_core(candidate, grid, density, step,
                                        common::SeriesQuantity<common::Frequency>::from_si(spatial::divergence(candidate.velocity().si_value(), grid)),
                                        options.projection.poisson)
        .state;
  };
  const auto rate = [&](const fluids::IncompressibleState &at, common::ScalarQuantity<common::Time> when) {
    return fluids::detail::rate_at(at, grid, when, rho, total, body, options.form);
  };
  const auto finalize = [&](const fluids::IncompressibleState &candidate) {
    auto checked = fluids::make_state(candidate.velocity(), candidate.pressure(), grid);
    return std::pair{checked, fluids::div_report(checked.velocity(), grid)};
  };
  return CoupledStep{fluids::detail::advance_stages(state, grid, time, step, std::move(limits), options, project_stage, rate, finalize), next};
}

} // namespace pluss::subsp::natsci::physics::fluids::rans
