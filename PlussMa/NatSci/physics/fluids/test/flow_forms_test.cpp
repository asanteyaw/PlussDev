#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

pluss::Series mesh(std::int64_t nx, std::int64_t ny, const auto &fn) {
  std::vector<double> values;
  values.reserve(static_cast<std::size_t>(nx * ny));
  for (std::int64_t x = 0; x < nx; ++x) {
    for (std::int64_t y = 0; y < ny; ++y) {
      values.push_back(fn(2.0 * std::numbers::pi * static_cast<double>(x) /
                              static_cast<double>(nx),
                          2.0 * std::numbers::pi * static_cast<double>(y) /
                              static_cast<double>(ny)));
    }
  }
  return pluss::series::make(values, {nx, ny});
}

spatial::PeriodicGrid periodic(std::int64_t nx, std::int64_t ny) {
  return spatial::make_periodic_grid(
      {nx, ny}, {common::ScalarQuantity<common::Length>::from_si(
                     2.0 * std::numbers::pi / static_cast<double>(nx)),
                 common::ScalarQuantity<common::Length>::from_si(
                     2.0 * std::numbers::pi / static_cast<double>(ny))});
}

pluss::Series velocity(std::int64_t nx, std::int64_t ny) {
  const pluss::Series u = mesh(nx, ny, [](double x, double y) {
    return 0.7 * std::sin(x) + 0.2 * std::cos(2.0 * y);
  });
  const pluss::Series v = mesh(nx, ny, [](double x, double y) {
    return -0.4 * std::cos(2.0 * x) + 0.5 * std::sin(y);
  });
  return u.unsqueeze(0).unite({v.unsqueeze(0)}, 0);
}

common::SeriesQuantity<common::Speed> speed(const pluss::Series &value) {
  return common::SeriesQuantity<common::Speed>::from_si(value);
}

void test_tensor_flux_contract() {
  constexpr std::int64_t nx = 9;
  constexpr std::int64_t ny = 7;
  const auto grid = periodic(nx, ny);
  const pluss::Series base = velocity(nx, ny);
  const pluss::Series flux = spatial::vector_flux(base, grid);
  expect(flux.sizes() == std::vector<std::int64_t>{2, 2, nx, ny},
         "vector_flux produced the wrong tensor layout");
  expect(flux.select(0, 0).select(0, 1).allclose(
             base.select(0, 0).mul(base.select(0, 1)), 1e-13, 1e-13),
         "vector_flux did not form the pointwise outer product");
  expect(spatial::tensor_div(flux, grid)
             .allclose(spatial::flux_div(base, grid), 1e-13, 1e-13),
         "flux_div is not the tensor divergence of u outer u");

  const pluss::Series lanes =
      base.unsqueeze(0).unite({base.mul(1.5).unsqueeze(0)}, 0);
  const pluss::Series batches = lanes.unsqueeze(0).unite(
      {lanes.mul(-0.5).unsqueeze(0), lanes.mul(2.0).unsqueeze(0)}, 0);
  expect(spatial::vector_flux(batches, grid).sizes() ==
             std::vector<std::int64_t>{3, 2, 2, 2, nx, ny},
         "vector_flux collapsed multidimensional batch axes");
  expect(
      spatial::tensor_div(spatial::vector_flux(batches, grid), grid).sizes() ==
          std::vector<std::int64_t>{3, 2, 2, nx, ny},
      "tensor_div collapsed multidimensional batch axes");
}

void test_periodic_forms_and_energy() {
  constexpr std::int64_t nx = 17;
  constexpr std::int64_t ny = 15;
  const auto grid = periodic(nx, ny);
  const pluss::Series base = velocity(nx, ny);
  const auto field = speed(base);
  const pluss::Series advect =
      fluids::flow_term(field, grid, fluids::FlowForm::Advect).si_value();
  const pluss::Series conserve =
      fluids::flow_term(field, grid, fluids::FlowForm::Conserve).si_value();
  const pluss::Series skew =
      fluids::flow_term(field, grid, fluids::FlowForm::Skew).si_value();

  expect(skew.allclose(advect.add(conserve).mul(0.5), 1e-13, 1e-13),
         "skew flow form is not the advertised split operator");
  const double energy_rate = base.mul(skew).sum().item<double>();
  expect(std::abs(energy_rate) < 2e-12,
         "periodic skew convection is not discretely energy neutral");
  expect(!advect.allclose(conserve, 1e-10, 1e-10),
         "the flow-form test did not distinguish non-solenoidal transport");
}

void test_flow_diagnostics() {
  constexpr std::int64_t nx = 9;
  constexpr std::int64_t ny = 7;
  const auto grid = periodic(nx, ny);
  const pluss::Series base = velocity(nx, ny);
  const auto field = speed(base);
  const auto rho = common::ScalarQuantity<fluids::Density>::from_si(1.4);
  const auto nu =
      common::ScalarQuantity<fluids::KinematicViscosity>::from_si(0.03);
  const auto mu =
      common::ScalarQuantity<fluids::DynamicViscosity>::from_si(0.042);

  const auto rate = fluids::strain(field, grid);
  expect(rate.si_value().sizes() == std::vector<std::int64_t>{2, 2, nx, ny},
         "strain produced the wrong tensor layout");
  expect(
      rate.si_value().allclose(rate.si_value().transpose(0, 1), 1e-13, 1e-13),
      "strain tensor is not symmetric");
  expect(fluids::vort(field, grid)
             .si_value()
             .allclose(spatial::curl(base, grid), 1e-13, 1e-13),
         "vort does not match the shared curl operator");
  expect(fluids::stress(field, grid, mu)
             .si_value()
             .allclose(rate.si_value().mul(2.0 * mu.si_value()), 1e-13, 1e-13),
         "viscous stress does not equal twice mu times strain");

  const auto stats = fluids::flow_stats(field, grid, rho, nu);
  expect(
      stats.kinetic_density.si_value().allclose(
          base.square().sum(0, false).mul(0.5 * rho.si_value()), 1e-13, 1e-13),
      "kinetic-energy density is incorrect");
  expect(stats.kinetic_mean.si_value().dim() == 0 &&
             stats.enstrophy_mean.si_value().dim() == 0 &&
             stats.dissipation_mean.si_value().dim() == 0 &&
             stats.speed_max.si_value().dim() == 0,
         "flow_stats did not reduce an unbatched field to scalar summaries");
  expect(stats.dissipation.si_value().ge(0.0).all().item<bool>() &&
             stats.dissipation_mean.si_value().ge(0.0).all().item<bool>(),
         "viscous dissipation became negative");

  const pluss::Series batched =
      base.unsqueeze(0).unite({base.mul(2.0).unsqueeze(0)}, 0);
  const auto batched_stats = fluids::flow_stats(speed(batched), grid, rho, nu);
  expect(batched_stats.kinetic_mean.si_value().sizes() ==
                 std::vector<std::int64_t>{2} &&
             batched_stats.enstrophy_mean.si_value().sizes() ==
                 std::vector<std::int64_t>{2} &&
             batched_stats.speed_max.si_value().sizes() ==
                 std::vector<std::int64_t>{2},
         "flow_stats collapsed batch lanes");
  expect(batched_stats.kinetic_mean.si_value().select(0, 1).allclose(
             batched_stats.kinetic_mean.si_value().select(0, 0).mul(4.0), 1e-12,
             1e-12),
         "flow_stats did not preserve quadratic batch scaling");
}

void test_variable_properties() {
  constexpr std::int64_t nx = 9;
  constexpr std::int64_t ny = 7;
  const auto grid = periodic(nx, ny);
  const pluss::Series psi = mesh(
      nx, ny, [](double x, double y) { return std::sin(x) * std::sin(y); });
  const pluss::Series solenoidal =
      spatial::partial(psi, grid, 1)
          .unsqueeze(0)
          .unite({spatial::partial(psi, grid, 0).neg().unsqueeze(0)}, 0);
  expect(spatial::divergence(solenoidal, grid).abs().max().item<double>() <
             1e-13,
         "variable-property test velocity is not discretely solenoidal");
  const pluss::Series zero_pressure =
      pluss::series::zeros({nx, ny}, solenoidal.scalar_type());
  const auto state = fluids::make_state(
      speed(solenoidal),
      common::SeriesQuantity<common::Pressure>::from_si(zero_pressure), grid);
  const auto rho_scalar = common::ScalarQuantity<fluids::Density>::from_si(1.4);
  const auto nu_scalar =
      common::ScalarQuantity<fluids::KinematicViscosity>::from_si(0.0);
  const auto rho_field = common::SeriesQuantity<fluids::Density>::from_si(
      pluss::series::full_like(zero_pressure, 1.4));
  const auto mu_field =
      common::SeriesQuantity<fluids::DynamicViscosity>::from_si(
          pluss::series::zeros_like(zero_pressure));
  expect(
      fluids::mom_rate(state, grid, rho_field, mu_field, fluids::FlowForm::Skew)
          .si_value()
          .allclose(fluids::mom_rate(state, grid, rho_scalar, nu_scalar,
                                     fluids::FlowForm::Skew)
                        .si_value(),
                    2e-12, 2e-12),
      "zero dynamic-viscosity fields do not recover the inviscid rate");
  const auto varying_mu =
      common::SeriesQuantity<fluids::DynamicViscosity>::from_si(
          pluss::series::full_like(zero_pressure, 0.042)
              .add(mesh(nx, ny, [](double x, double y) {
                return 0.004 * (1.0 + std::sin(x) * std::cos(y));
              })));
  const pluss::Series viscous_rate =
      fluids::mom_rate(state, grid, rho_field, varying_mu,
                       fluids::FlowForm::Skew)
          .si_value();
  expect(viscous_rate.sizes() == solenoidal.sizes() &&
             viscous_rate.isfinite().all().item<bool>(),
         "variable viscous-stress divergence lost layout or finiteness");
  expect(
      fluids::stress(speed(solenoidal), grid, varying_mu).si_value().sizes() ==
          std::vector<std::int64_t>{2, 2, nx, ny},
      "variable viscous stress produced the wrong tensor layout");

  const pluss::Series rho_values = mesh(nx, ny, [](double x, double y) {
    return 1.2 + 0.1 * std::sin(x) * std::cos(y);
  });
  const pluss::Series phi = mesh(nx, ny, [](double x, double y) {
    return std::sin(x) + 0.3 * std::cos(y);
  });
  const double dt = 0.2;
  const pluss::Series weighted_gradient =
      spatial::gradient(phi, grid).div(rho_values.unsqueeze(0)).mul(dt);
  const auto gradient_state = fluids::make_state(
      speed(weighted_gradient),
      common::SeriesQuantity<common::Pressure>::from_si(zero_pressure), grid);
  fluids::ProjectionOptions options;
  options.poisson.solve.max_iterations = 256;
  options.poisson.solve.check_interval = 1;
  options.poisson.solve.relative_tolerance = 1e-11;
  options.poisson.solve.absolute_tolerance = 1e-12;
  const auto projected = fluids::project(
      gradient_state, grid,
      common::SeriesQuantity<fluids::Density>::from_si(rho_values),
      common::ScalarQuantity<common::Time>::from_si(dt), options);
  fluids::ProjectionOptions unscaled_options = options;
  unscaled_options.poisson.pc = spatial::PoissonPc::None;
  const auto unscaled_projection = fluids::project(
      gradient_state, grid,
      common::SeriesQuantity<fluids::Density>::from_si(rho_values),
      common::ScalarQuantity<common::Time>::from_si(dt), unscaled_options);
  expect(projected.state.velocity().si_value().abs().max().item<double>() <
             2e-9,
         "variable-density projection did not remove a weighted gradient");
  expect(projected.pressure_correction.si_value().allclose(phi, 2e-9, 2e-9),
         "variable-density projection recovered the wrong pressure gauge");
  expect(projected.after.max_abs.si_value().item<double>() < 2e-9,
         "variable-density projection left a divergent velocity");
  expect(projected.pressure_correction.si_value().allclose(
             unscaled_projection.pressure_correction.si_value(), 2e-9, 2e-9) &&
             projected.iterations <= unscaled_projection.iterations,
         "diagonal pressure scaling changed the solution or increased the "
         "Krylov iteration count");

  fluids::StepOptions step_options;
  step_options.rule = fluids::StepRule::Rk2;
  step_options.form = fluids::FlowForm::Skew;
  step_options.guard.enforce = false;
  step_options.projection = options;
  const auto stepped = fluids::advance(
      state, grid, common::SeriesQuantity<fluids::Density>::from_si(rho_values),
      varying_mu, common::ScalarQuantity<common::Time>::from_si(0.0),
      common::ScalarQuantity<common::Time>::from_si(1e-3), step_options);
  expect(stepped.state.velocity().si_value().sizes() == solenoidal.sizes() &&
             stepped.state.velocity().si_value().isfinite().all().item<bool>(),
         "variable-property step lost velocity layout or finiteness");
  expect(stepped.divergence.max_abs.si_value().item<double>() < 2e-8,
         "variable-property step did not enforce incompressibility");
  const auto limits = fluids::step_limit(
      state, grid, common::SeriesQuantity<fluids::Density>::from_si(rho_values),
      varying_mu);
  expect(limits.allowed.si_value().dim() == 0 &&
             limits.allowed.si_value().gt(0.0).item<bool>(),
         "variable-property stability limit is not a positive scalar");

  bool rejected = false;
  try {
    (void)fluids::project(
        gradient_state, grid,
        common::SeriesQuantity<fluids::Density>::from_si(rho_values.index_fill(
            0, pluss::series::make({0}, pluss::ScalarType::Int64), 0.0)),
        common::ScalarQuantity<common::Time>::from_si(dt), options);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  expect(rejected, "variable-density projection accepted a zero-density cell");
}

void test_bounded_forms_and_step_wiring() {
  constexpr std::int64_t nx = 5;
  constexpr std::int64_t ny = 4;
  const auto grid = spatial::make_box_grid(
      {nx, ny}, {common::ScalarQuantity<common::Length>::from_si(0.2),
                 common::ScalarQuantity<common::Length>::from_si(0.25)});
  const auto bounds =
      fluids::flow_bounds(grid, {fluids::no_slip({0, spatial::Side::Low}),
                                 fluids::outlet({0, spatial::Side::High}),
                                 fluids::free_slip({1, spatial::Side::Low}),
                                 fluids::outlet({1, spatial::Side::High})});
  const pluss::Series base =
      pluss::series::make(std::vector<double>(2 * nx * ny, 0.75), {2, nx, ny});
  for (const auto form : {fluids::FlowForm::Advect, fluids::FlowForm::Conserve,
                          fluids::FlowForm::Skew}) {
    const pluss::Series term =
        fluids::flow_term(speed(base), grid, bounds, form).si_value();
    expect(term.sizes() == base.sizes() && term.isfinite().all().item<bool>(),
           "bounded flow form lost layout or produced non-finite values");
  }

  const auto pgrid = periodic(nx, ny);
  const auto state = fluids::make_state(
      speed(base),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::zeros({nx, ny}, base.scalar_type())),
      pgrid);
  fluids::StepOptions advect_options;
  advect_options.rule = fluids::StepRule::Euler;
  advect_options.form = fluids::FlowForm::Advect;
  advect_options.guard.enforce = false;
  fluids::StepOptions skew_options = advect_options;
  skew_options.form = fluids::FlowForm::Skew;
  const auto rho = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto nu =
      common::ScalarQuantity<fluids::KinematicViscosity>::from_si(0.0);
  const auto time = common::ScalarQuantity<common::Time>::from_si(0.0);
  const auto step = common::ScalarQuantity<common::Time>::from_si(1e-3);
  const auto advected =
      fluids::advance(state, pgrid, rho, nu, time, step, advect_options);
  const auto skewed =
      fluids::advance(state, pgrid, rho, nu, time, step, skew_options);
  expect(advected.state.velocity().si_value().allclose(
             skewed.state.velocity().si_value(), 1e-13, 1e-13),
         "uniform periodic flow should be invariant under every flow form");
}

void test_bounded_variable_properties() {
  constexpr std::int64_t nx = 7;
  constexpr std::int64_t ny = 6;
  const auto grid = spatial::make_box_grid(
      {nx, ny}, {common::ScalarQuantity<common::Length>::from_si(0.2),
                 common::ScalarQuantity<common::Length>::from_si(0.25)});
  const auto bounds =
      fluids::flow_bounds(grid, {fluids::no_slip({0, spatial::Side::Low}),
                                 fluids::no_slip({0, spatial::Side::High}),
                                 fluids::no_slip({1, spatial::Side::Low}),
                                 fluids::no_slip({1, spatial::Side::High})});
  const pluss::Series rho_value = mesh(nx, ny, [](double x, double y) {
    return 1.1 + 0.15 * (1.0 + std::sin(x) * std::cos(y));
  });
  const pluss::Series zero =
      pluss::series::zeros({nx, ny}, rho_value.scalar_type());
  const pluss::Series mu_value = mesh(nx, ny, [](double x, double y) {
    return 0.03 + 0.01 * (1.0 + std::cos(x) * std::cos(y));
  });
  const auto rho = common::SeriesQuantity<fluids::Density>::from_si(rho_value);
  const auto mu =
      common::SeriesQuantity<fluids::DynamicViscosity>::from_si(mu_value);
  const pluss::Series phi = mesh(
      nx, ny, [](double x, double y) { return std::cos(x) * std::cos(y); });
  const auto correction_bounds = fluids::p_bounds(grid, bounds).zeroed();
  const double dt = 0.05;
  const pluss::Series weighted_gradient =
      spatial::gradient(common::SeriesQuantity<common::Pressure>::from_si(phi),
                        grid, correction_bounds)
          .si_value()
          .div(rho_value.unsqueeze(0))
          .mul(dt);
  const auto gradient_state = fluids::make_state(
      speed(weighted_gradient),
      common::SeriesQuantity<common::Pressure>::from_si(zero), grid);

  fluids::ProjectionOptions projection_options;
  projection_options.poisson.solve.max_iterations = 512;
  projection_options.poisson.solve.check_interval = 1;
  projection_options.poisson.solve.relative_tolerance = 1e-11;
  projection_options.poisson.solve.absolute_tolerance = 1e-12;
  const auto projected = fluids::project(
      gradient_state, grid, bounds, rho,
      common::ScalarQuantity<common::Time>::from_si(dt), projection_options);
  expect(projected.after.max_abs.si_value().item<double>() < 2e-9,
         "bounded variable-density projection left a divergent velocity");
  expect(projected.state.velocity().si_value().abs().max().item<double>() <
             2e-9,
         "bounded variable-density projection did not remove its weighted "
         "gradient");
  const auto axes = spatial::detail::spatial_axes(phi, grid);
  expect(projected.pressure_correction.si_value().allclose(
             phi.sub(phi.mean(axes, true)), 2e-9, 2e-9),
         "bounded variable-density projection recovered the wrong gauge");

  const auto state = fluids::make_state(
      speed(velocity(nx, ny)),
      common::SeriesQuantity<common::Pressure>::from_si(zero), grid);
  const pluss::Series rate =
      fluids::mom_rate(state, grid, bounds, rho, mu, fluids::FlowForm::Skew)
          .si_value();
  expect(rate.sizes() == state.velocity().si_value().sizes() &&
             rate.isfinite().all().item<bool>(),
         "bounded variable stress divergence lost layout or finiteness");
  expect(
      fluids::stress(state.velocity(), grid, bounds, mu).si_value().sizes() ==
          std::vector<std::int64_t>{2, 2, nx, ny},
      "bounded variable stress produced the wrong tensor layout");

  fluids::StepOptions step_options;
  step_options.rule = fluids::StepRule::Euler;
  step_options.form = fluids::FlowForm::Skew;
  step_options.guard.enforce = false;
  step_options.projection = projection_options;
  const auto stepped = fluids::advance(
      state, grid, bounds, rho, mu,
      common::ScalarQuantity<common::Time>::from_si(0.0),
      common::ScalarQuantity<common::Time>::from_si(1e-4), step_options);
  expect(stepped.state.velocity().si_value().isfinite().all().item<bool>() &&
             stepped.divergence.max_abs.si_value().item<double>() < 2e-8,
         "bounded variable-property step did not remain finite and "
         "divergence-free");
  const auto path = fluids::evolve(
      state, grid, bounds, rho, mu,
      common::ScalarQuantity<common::Time>::from_si(0.0),
      common::ScalarQuantity<common::Time>::from_si(1e-4), 2, step_options);
  expect(path.steps == 2 && path.rate_evaluations == 2 && path.projections == 2,
         "bounded variable-property trajectory reported wrong work counts");

  fluids::RunOptions run_options;
  run_options.max_step = common::ScalarQuantity<common::Time>::from_si(4e-5);
  run_options.step = step_options;
  const auto adaptive = fluids::evolve_to(
      state, grid, bounds, rho, mu,
      common::ScalarQuantity<common::Time>::from_si(0.0),
      common::ScalarQuantity<common::Time>::from_si(1e-4), run_options);
  expect(adaptive.steps == 3 && adaptive.rate_evaluations == 3 &&
             adaptive.projections == 3,
         "adaptive bounded flow did not honor its accepted-step budget");
  expect(adaptive.trajectory.coordinate().si_value().allclose(
             pluss::series::make({0.0, 4e-5, 8e-5, 1e-4}), 1e-14, 1e-14),
         "adaptive bounded flow did not clamp its final step to stop");
}

void test_implicit_diffusion() {
  constexpr std::int64_t nx = 9;
  constexpr std::int64_t ny = 7;
  const auto grid = periodic(nx, ny);
  const pluss::Series shear =
      mesh(nx, ny, [](double, double y) { return std::sin(y); });
  const pluss::Series velocity_field = shear.unsqueeze(0).unite(
      {pluss::series::zeros_like(shear).unsqueeze(0)}, 0);
  const auto state =
      fluids::make_state(speed(velocity_field),
                         common::SeriesQuantity<common::Pressure>::from_si(
                             pluss::series::zeros_like(shear)),
                         grid);
  const auto rho = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto nu =
      common::ScalarQuantity<fluids::KinematicViscosity>::from_si(5.0);
  const auto time = common::ScalarQuantity<common::Time>::from_si(0.0);
  const auto step = common::ScalarQuantity<common::Time>::from_si(0.1);

  fluids::StepOptions explicit_options;
  explicit_options.rule = fluids::StepRule::Euler;
  bool explicit_rejected = false;
  try {
    (void)fluids::advance(state, grid, rho, nu, time, step, explicit_options);
  } catch (const std::domain_error &) {
    explicit_rejected = true;
  }
  expect(explicit_rejected,
         "explicit diffusion accepted a step above its viscous limit");

  fluids::StepOptions implicit_options;
  implicit_options.rule = fluids::StepRule::Imex1;
  implicit_options.implicit.max_iterations = 128;
  implicit_options.implicit.check_interval = 1;
  implicit_options.implicit.relative_tolerance = 1e-12;
  implicit_options.implicit.absolute_tolerance = 1e-13;
  const auto result =
      fluids::advance(state, grid, rho, nu, time, step, implicit_options);
  const double spacing = grid.spacing(1).si_value();
  const double wave = 2.0 * std::numbers::pi / static_cast<double>(ny);
  const double eigenvalue =
      -4.0 * std::pow(std::sin(0.5 * wave), 2.0) / (spacing * spacing);
  const double factor =
      1.0 / (1.0 - step.si_value() * nu.si_value() * eigenvalue);
  expect(result.state.velocity().si_value().allclose(velocity_field.mul(factor),
                                                     2e-11, 2e-11),
         "Imex1 did not solve the periodic backward-Euler diffusion mode");
  expect(result.linear_iterations > 0 && result.rate_evaluations == 1 &&
             result.projections == 1,
         "Imex1 did not report its implicit and projection work");
  expect(result.divergence.max_abs.si_value().item<double>() < 2e-11,
         "Imex1 left a divergent periodic velocity");
}

void test_reverse_and_failures() {
  const auto grid = periodic(9, 7);
  pluss::Series tracked = velocity(9, 7);
  tracked.tracks_slope_(true);
  spatial::skew_advect(tracked, grid).square().sum().reverse();
  expect(tracked.has_sensitivity() &&
             tracked.sensitivity().isfinite().all().item<bool>(),
         "skew convection severed reverse sensitivity");

  bool rejected = false;
  try {
    (void)spatial::tensor_div(pluss::series::zeros({2, 3, 9, 7}), grid);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  expect(rejected, "tensor_div accepted mismatched tensor component axes");
}

} // namespace

int main() {
  try {
    test_tensor_flux_contract();
    test_periodic_forms_and_energy();
    test_flow_diagnostics();
    test_variable_properties();
    test_bounded_forms_and_step_wiring();
    test_bounded_variable_properties();
    test_implicit_diffusion();
    test_reverse_and_failures();
    std::cout << "NatSci fluid flow-form checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
