#pragma once

// Large-eddy simulation closures for the incompressible solver: eddy viscosity nu_t from the resolved velocity gradient g_ij = d u_i / d x_j, entering the
// momentum equation through the existing variable-viscosity stress divergence (so the model is conservative and the stress is 2 (nu + nu_t) S).
//
//  * Smagorinsky:  nu_t = (C_s Delta)^2 |S|,  |S| = sqrt(2 S_ij S_ij),  C_s = 0.17 by default.
//  * WALE (Nicoud & Ducros, Flow Turb. Combust. 62 (1999) 183): nu_t = (C_w Delta)^2 (S^d_ij S^d_ij)^(3/2) / ((S_ij S_ij)^(5/2) + (S^d_ij S^d_ij)^(5/4)),
//    S^d_ij = (g_ik g_kj + g_jk g_ki)/2 - delta_ij g_kl g_lk / 3,  C_w = 0.325 (the OpenFOAM / commercial default, C_w^2 ~ 10.6 C_s^2). Vanishes in pure shear
//    and scales as y^3 at a wall without damping functions. The 1/3 is the 3-D definition and is kept in 2-D.
//  * Dynamic Smagorinsky (Germano et al. 1991, Lilly 1992), periodic grids: C_s^2 from the Germano identity with a (1/4, 1/2, 1/4) test filter of
//    equivalent width sqrt(6) Delta, C_s^2 = <L_ij M_ij> / <M_ij M_ij>, averaged over the whole field per batch lane (or filtered locally), clipped at zero.
//  Delta defaults to the geometric mean of the grid spacings. No van Driest damping is applied to Smagorinsky near walls (use WALE or dynamic there).
//  Constants and formulas follow the cited papers as reported by secondary sources (the Ducros-Nicoud-Poinsot WALE note for S^d); the original papers
//  were not obtained.

#include "pluss/subsp/natsci/physics/fluids.h"

#include <cmath>
#include <numbers>

namespace pluss::subsp::natsci::physics::fluids::les {

enum class Model : std::uint8_t { Smagorinsky = 0, Wale = 1, DynamicSmagorinsky = 2 };

struct Closure final {
  Model model{Model::Wale};
  double constant{-1.0};   // C_s or C_w; negative selects the model default (dynamic ignores it)
  double width{0.0};       // filter width Delta [m]; 0 = geometric mean of the grid spacings
  bool clip{true};         // dynamic model: no backscatter (C_s^2 >= 0)
  bool local_average{false}; // dynamic model: test-filter the numerator and denominator instead of a global average
  double test_ratio{std::numbers::sqrt2 * std::numbers::sqrt3}; // test-filter width / Delta (second-moment equivalent width of the 1/4,1/2,1/4 stencil)
};

namespace detail {

template <class Grid> double filter_width(const Grid &grid, const Closure &closure) {
  if (closure.width > 0.0)
    return closure.width;
  double product = 1.0;
  for (std::int64_t a = 0; a < grid.dim(); ++a)
    product *= grid.spacing(a).si_value();
  return std::pow(product, 1.0 / static_cast<double>(grid.dim()));
}
inline double constant_of(const Closure &c) {
  if (c.constant >= 0.0)
    return c.constant;
  return c.model == Model::Smagorinsky ? 0.17 : 0.325;
}

// g[i][j] fields from the gradient tensor [..., i, j, space...]
inline std::vector<std::vector<Series>> components(const Series &gradient, std::int64_t component_axis, std::int64_t d) {
  std::vector<std::vector<Series>> g(static_cast<std::size_t>(d), std::vector<Series>(static_cast<std::size_t>(d)));
  for (std::int64_t i = 0; i < d; ++i)
    for (std::int64_t j = 0; j < d; ++j)
      g[i][j] = gradient.select(component_axis, i).select(component_axis, j);
  return g;
}
inline Series strain_norm(const std::vector<std::vector<Series>> &g) { // |S| = sqrt(2 S_ij S_ij)
  const std::size_t d = g.size();
  Series sum = pluss::series::undefined();
  for (std::size_t i = 0; i < d; ++i)
    for (std::size_t j = 0; j < d; ++j) {
      const Series s = g[i][j].add(g[j][i]).mul(0.5);
      sum = sum.defined() ? sum.add(s.square()) : s.square();
    }
  return sum.mul(2.0).sqrt();
}
inline Series smagorinsky(const std::vector<std::vector<Series>> &g, double delta, double cs) { return strain_norm(g).mul(cs * cs * delta * delta); }
inline Series wale(const std::vector<std::vector<Series>> &g, double delta, double cw) {
  const std::size_t d = g.size();
  std::vector<std::vector<Series>> g2(d, std::vector<Series>(d));
  Series trace = pluss::series::undefined(), s2 = pluss::series::undefined();
  for (std::size_t i = 0; i < d; ++i)
    for (std::size_t j = 0; j < d; ++j) {
      Series acc = pluss::series::undefined();
      for (std::size_t k = 0; k < d; ++k) {
        const Series term = g[i][k].mul(g[k][j]);
        acc = acc.defined() ? acc.add(term) : term;
      }
      g2[i][j] = acc;
      const Series s = g[i][j].add(g[j][i]).mul(0.5);
      s2 = s2.defined() ? s2.add(s.square()) : s.square();
    }
  for (std::size_t k = 0; k < d; ++k)
    trace = trace.defined() ? trace.add(g2[k][k]) : g2[k][k];
  Series sd2 = pluss::series::undefined();
  for (std::size_t i = 0; i < d; ++i)
    for (std::size_t j = 0; j < d; ++j) {
      Series sd = g2[i][j].add(g2[j][i]).mul(0.5);
      if (i == j)
        sd = sd.sub(trace.mul(1.0 / 3.0));
      sd2 = sd2.defined() ? sd2.add(sd.square()) : sd.square();
    }
  const Series numerator = sd2.pow(1.5);
  const Series denominator = s2.pow(2.5).add(sd2.pow(1.25));
  const Series safe = denominator.gt(0.0).where(denominator, denominator.mul(0.0).add(1.0));
  return numerator.div(safe).mul(cw * cw * delta * delta);
}

// (1/4, 1/2, 1/4) test filter on the periodic spatial axes (the last d dims)
inline Series test_filter(const Series &x, std::int64_t d) {
  Series y = x;
  for (std::int64_t a = 0; a < d; ++a) {
    const std::int64_t axis = x.dim() - d + a;
    y = y.mul(0.5).add(y.roll(1, axis).add(y.roll(-1, axis)).mul(0.25));
  }
  return y;
}
inline Series spatial_mean(const Series &x, std::int64_t d) {
  std::vector<std::int64_t> axes;
  for (std::int64_t a = 0; a < d; ++a)
    axes.push_back(x.dim() - d + a);
  return x.mean(axes, true);
}

inline Series dynamic_smagorinsky(const Series &velocity, const std::vector<std::vector<Series>> &g, std::int64_t component_axis, std::int64_t d, double delta, const Closure &closure) {
  const std::size_t n = static_cast<std::size_t>(d);
  const Series smag = strain_norm(g);
  std::vector<Series> u(n), ubar(n);
  for (std::size_t i = 0; i < n; ++i) {
    u[i] = velocity.select(component_axis, static_cast<std::int64_t>(i));
    ubar[i] = test_filter(u[i], d);
  }
  std::vector<std::vector<Series>> s(n, std::vector<Series>(n)), sbar(n, std::vector<Series>(n));
  Series sbar2 = pluss::series::undefined();
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j) {
      s[i][j] = g[i][j].add(g[j][i]).mul(0.5);
      sbar[i][j] = test_filter(s[i][j], d);
      sbar2 = sbar2.defined() ? sbar2.add(sbar[i][j].square()) : sbar[i][j].square();
    }
  const Series sbar_norm = sbar2.mul(2.0).sqrt();
  // Leonard stress (deviatoric) and the model difference tensor
  std::vector<std::vector<Series>> leonard(n, std::vector<Series>(n)), model(n, std::vector<Series>(n));
  Series leonard_trace = pluss::series::undefined();
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j) {
      leonard[i][j] = test_filter(u[i].mul(u[j]), d).sub(ubar[i].mul(ubar[j]));
      model[i][j] = sbar_norm.mul(sbar[i][j]).mul(closure.test_ratio * closure.test_ratio).sub(test_filter(smag.mul(s[i][j]), d)).mul(-2.0 * delta * delta);
    }
  for (std::size_t k = 0; k < n; ++k)
    leonard_trace = leonard_trace.defined() ? leonard_trace.add(leonard[k][k]) : leonard[k][k];
  Series lm = pluss::series::undefined(), mm = pluss::series::undefined();
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j) {
      Series l = leonard[i][j];
      if (i == j)
        l = l.sub(leonard_trace.mul(1.0 / static_cast<double>(d)));
      const Series a = l.mul(model[i][j]), b = model[i][j].square();
      lm = lm.defined() ? lm.add(a) : a;
      mm = mm.defined() ? mm.add(b) : b;
    }
  if (closure.local_average) {
    lm = test_filter(lm, d);
    mm = test_filter(mm, d);
  } else {
    lm = spatial_mean(lm, d);
    mm = spatial_mean(mm, d);
  }
  const Series safe = mm.gt(0.0).where(mm, mm.mul(0.0).add(1.0));
  Series cs2 = lm.div(safe).mul(mm.gt(0.0).to(mm.scalar_type()));
  if (closure.clip)
    cs2 = cs2.clamp_min(0.0);
  return cs2.mul(smag).mul(delta * delta);
}

inline void require_model(const Closure &c, bool periodic) {
  if (c.constant < -1.0 || !std::isfinite(c.constant) || c.width < 0.0 || !std::isfinite(c.width) || !(c.test_ratio > 1.0))
    throw std::invalid_argument("natsci::fluids::les: closure constant, width and test ratio must be finite (width >= 0, ratio > 1)");
  if (c.model == Model::DynamicSmagorinsky && !periodic)
    throw std::invalid_argument("natsci::fluids::les: the dynamic model needs homogeneous directions (periodic grids)");
}

} // namespace detail

// eddy viscosity of the resolved field [m^2/s], shape of the pressure field
[[nodiscard]] inline common::SeriesQuantity<KinematicViscosity> eddy_viscosity(const IncompressibleState &state, const spatial::PeriodicGrid &grid, const Closure &closure = {}) {
  detail::require_model(closure, true);
  const Series &velocity = state.velocity().si_value();
  const std::int64_t ca = velocity.dim() - grid.dim() - 1, d = grid.dim();
  const auto g = detail::components(spatial::gradient(velocity, grid), ca + 0, d);
  const double delta = detail::filter_width(grid, closure);
  Series nut = closure.model == Model::Smagorinsky ? detail::smagorinsky(g, delta, detail::constant_of(closure))
               : closure.model == Model::Wale      ? detail::wale(g, delta, detail::constant_of(closure))
                                                    : detail::dynamic_smagorinsky(velocity, g, ca, d, delta, closure);
  return common::SeriesQuantity<KinematicViscosity>::from_si(nut.expand(state.pressure().si_value().sizes()).contiguous());
}
[[nodiscard]] inline common::SeriesQuantity<KinematicViscosity> eddy_viscosity(const IncompressibleState &state, const spatial::BoxGrid &grid, const FlowBounds &bounds, const Closure &closure = {}) {
  detail::require_model(closure, false);
  const std::int64_t ca = state.velocity().si_value().dim() - grid.dim() - 1, d = grid.dim();
  const auto g = detail::components(fluids::detail::vel_grad(state.velocity(), grid, bounds), ca, d);
  const double delta = detail::filter_width(grid, closure);
  const Series nut = closure.model == Model::Smagorinsky ? detail::smagorinsky(g, delta, detail::constant_of(closure)) : detail::wale(g, delta, detail::constant_of(closure));
  return common::SeriesQuantity<KinematicViscosity>::from_si(nut);
}

// mean subgrid dissipation <2 nu_t S_ij S_ij> over the spatial axes [m^2/s^3] (per batch lane)
[[nodiscard]] inline Series sgs_dissipation(const IncompressibleState &state, const spatial::PeriodicGrid &grid, const Closure &closure = {}) {
  const Series nut = eddy_viscosity(state, grid, closure).si_value();
  const std::int64_t ca = state.velocity().si_value().dim() - grid.dim() - 1;
  const auto g = detail::components(spatial::gradient(state.velocity().si_value(), grid), ca, grid.dim());
  const Series s = detail::strain_norm(g);
  return detail::spatial_mean(nut.mul(s.square()), grid.dim()); // 2 S_ij S_ij = |S|^2
}

// One step with the closure re-evaluated from every stage state (the eddy viscosity is part of the momentum rate, so RK stages see the model consistently).
[[nodiscard]] inline StepResult advance(const IncompressibleState &state, const spatial::PeriodicGrid &grid, const common::ScalarQuantity<Density> &density,
                                        const common::ScalarQuantity<KinematicViscosity> &viscosity, const Closure &closure,
                                        common::ScalarQuantity<common::Time> time, common::ScalarQuantity<common::Time> step,
                                        const StepOptions &options = {}, const BodyFn &body = {}) {
  if (options.rule == StepRule::Imex1)
    throw std::invalid_argument("natsci::fluids::les::advance: Imex1 needs scalar viscosity; use an explicit rule with a closure");
  const Series ones = pluss::series::zeros_like(state.pressure().si_value()).add(1.0);
  const auto rho = common::SeriesQuantity<Density>::from_si(ones.mul(density.si_value()));
  const auto total = [&](const IncompressibleState &at) {
    return common::SeriesQuantity<DynamicViscosity>::from_si(eddy_viscosity(at, grid, closure).si_value().add(viscosity.si_value()).mul(density.si_value()));
  };
  StepLimits limits = fluids::detail::prepare_step(state, grid, rho, total(state), time, step, options);
  const auto project_stage = [&](const IncompressibleState &candidate) {
    return fluids::detail::project_core(candidate, grid, density, step,
                                        common::SeriesQuantity<common::Frequency>::from_si(spatial::divergence(candidate.velocity().si_value(), grid)),
                                        options.projection.poisson)
        .state;
  };
  const auto rate = [&](const IncompressibleState &at, common::ScalarQuantity<common::Time> when) {
    return fluids::detail::rate_at(at, grid, when, rho, total(at), body, options.form);
  };
  const auto finalize = [&](const IncompressibleState &candidate) {
    auto checked = make_state(candidate.velocity(), candidate.pressure(), grid);
    return std::pair{checked, div_report(checked.velocity(), grid)};
  };
  return fluids::detail::advance_stages(state, grid, time, step, std::move(limits), options, project_stage, rate, finalize);
}

[[nodiscard]] inline StepResult advance(const IncompressibleState &state, const spatial::BoxGrid &grid, const FlowBounds &bounds, const common::ScalarQuantity<Density> &density,
                                        const common::ScalarQuantity<KinematicViscosity> &viscosity, const Closure &closure,
                                        common::ScalarQuantity<common::Time> time, common::ScalarQuantity<common::Time> step,
                                        const StepOptions &options = {}, const BodyFn &body = {}) {
  if (options.rule == StepRule::Imex1)
    throw std::invalid_argument("natsci::fluids::les::advance: Imex1 needs scalar viscosity; use an explicit rule with a closure");
  const Series ones = pluss::series::zeros_like(state.pressure().si_value()).add(1.0);
  const auto rho = common::SeriesQuantity<Density>::from_si(ones.mul(density.si_value()));
  const auto total = [&](const IncompressibleState &at) {
    return common::SeriesQuantity<DynamicViscosity>::from_si(eddy_viscosity(at, grid, bounds, closure).si_value().add(viscosity.si_value()).mul(density.si_value()));
  };
  StepLimits limits = fluids::detail::prepare_step(state, grid, rho, total(state), time, step, options);
  const auto project_stage = [&](const IncompressibleState &candidate) {
    return fluids::detail::project_core(candidate, grid, bounds, density, step,
                                        common::SeriesQuantity<common::Frequency>::from_si(fluids::detail::vel_divergence(candidate.velocity(), grid, bounds)),
                                        options.projection.poisson)
        .state;
  };
  const auto rate = [&](const IncompressibleState &at, common::ScalarQuantity<common::Time> when) {
    return fluids::detail::rate_at(at, grid, bounds, when, rho, total(at), body, options.form);
  };
  const auto finalize = [&](const IncompressibleState &candidate) {
    auto checked = make_state(candidate.velocity(), candidate.pressure(), grid);
    return std::pair{checked, div_report(checked.velocity(), grid, bounds)};
  };
  return fluids::detail::advance_stages(state, grid, time, step, std::move(limits), options, project_stage, rate, finalize);
}

} // namespace pluss::subsp::natsci::physics::fluids::les
