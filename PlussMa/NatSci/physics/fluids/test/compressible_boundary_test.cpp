#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace gas = pluss::subsp::natsci::physics::fluids::gas;

using pluss::ScalarType;
using pluss::Series;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

template <class Fn> void expect_failure(Fn &&fn, const std::string &message) {
  try {
    std::forward<Fn>(fn)();
  } catch (const std::exception &) {
    return;
  }
  throw std::runtime_error(message);
}

common::ScalarQuantity<common::Time> seconds(double value) {
  return common::ScalarQuantity<common::Time>::from_si(value);
}

common::ScalarQuantity<common::Length> metres(double value) {
  return common::ScalarQuantity<common::Length>::from_si(value);
}

spatial::BoxGrid grid1(std::int64_t cells, double width) {
  return spatial::make_box_grid({cells}, {metres(width)});
}

spatial::BoxGrid grid2(std::int64_t nx, std::int64_t ny, double dx, double dy) {
  return spatial::make_box_grid({nx, ny}, {metres(dx), metres(dy)});
}

gas::Primitive primitive_data(const Series &rho, const Series &velocity,
                              const Series &pressure) {
  return gas::primitive(
      common::SeriesQuantity<gas::Density>::from_si(rho),
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(pressure));
}

gas::Primitive scalar_primitive(double rho, std::initializer_list<double> u,
                                double pressure) {
  return primitive_data(pluss::series::full({}, rho, ScalarType::Float64),
                        pluss::series::make(u, ScalarType::Float64),
                        pluss::series::full({}, pressure, ScalarType::Float64));
}

gas::State make_state(const Series &rho, const Series &velocity,
                      const Series &pressure, const spatial::BoxGrid &grid,
                      const gas::Model &model = {}) {
  const std::int64_t component = rho.dim() - grid.dim();
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series kinetic =
      velocity.square().sum(component, false).mul(rho).mul(0.5);
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(
                       pressure.div(model.gamma - 1.0).add(kinetic)),
                   grid, model);
}

gas::Bounds fixed_ends(const spatial::BoxGrid &grid,
                       const gas::Primitive &state) {
  return gas::bounds(grid, {gas::fixed({0, spatial::Side::Low}, state),
                            gas::fixed({0, spatial::Side::High}, state)});
}

gas::State final_state(const gas::FlowPath &path, const spatial::BoxGrid &grid,
                       const gas::Model &model = {}) {
  const auto &record = path.trajectory.state();
  return gas::make(
      common::SeriesQuantity<gas::Density>::from_si(
          record.get<gas::Density>("density").si_value().select(0, -1)),
      common::SeriesQuantity<gas::MomDensity>::from_si(
          record.get<gas::MomDensity>("momentum").si_value().select(0, -1)),
      common::SeriesQuantity<gas::EnergyDensity>::from_si(
          record.get<gas::EnergyDensity>("energy").si_value().select(0, -1)),
      grid, model);
}

void test_fixed_state_and_pressure_outlet() {
  constexpr std::int64_t n = 18;
  const auto grid = grid1(n, 0.05);
  const Series rho = pluss::series::full({2, n}, 1.3, ScalarType::Float64);
  const Series speed =
      pluss::series::full({2, 1, n}, 0.25, ScalarType::Float64);
  const Series pressure = pluss::series::full({2, n}, 0.9, ScalarType::Float64);
  const auto state = make_state(rho, speed, pressure, grid);
  const auto prescribed = scalar_primitive(1.3, {0.25}, 0.9);

  gas::Options options;
  options.recon = gas::Recon::Minmod;
  options.flux = gas::FluxScheme::Hllc;
  const auto stationary =
      gas::rate(state, grid, fixed_ends(grid, prescribed), {}, options);
  expect(stationary.density.si_value().abs().max().item<double>() < 1e-14 &&
             stationary.momentum.si_value().abs().max().item<double>() <
                 1e-14 &&
             stationary.energy.si_value().abs().max().item<double>() < 1e-14,
         "a matching fixed boundary did not preserve a batched uniform state");

  const auto outlet_rules = gas::bounds(
      grid,
      {gas::fixed({0, spatial::Side::Low}, prescribed),
       gas::outlet({0, spatial::Side::High},
                   common::SeriesQuantity<common::Pressure>::from_si(
                       pluss::series::full({}, 0.9, ScalarType::Float64)))});
  const auto outlet_rate = gas::rate(state, grid, outlet_rules, {}, options);
  expect(outlet_rate.density.si_value().abs().max().item<double>() < 1e-14 &&
             outlet_rate.momentum.si_value().abs().max().item<double>() <
                 1e-14 &&
             outlet_rate.energy.si_value().abs().max().item<double>() < 1e-14,
         "a matching pressure outlet disturbed a uniform stream");
}

void test_slip_conservation_and_tangential_flow() {
  constexpr std::int64_t n = 24;
  const auto grid = grid1(n, 1.0 / n);
  const Series x = pluss::series::arange(0, n, 1, ScalarType::Float64).add(0.5);
  const Series rho = x.mul(0.2 / n).add(1.0);
  const Series pressure = x.mul(-0.1 / n).add(1.2);
  const Series speed = x.mul(0.3 / n).sub(0.15).unsqueeze(0);
  const auto state = make_state(rho, speed, pressure, grid);
  const auto closed = gas::bounds(grid, {gas::slip({0, spatial::Side::Low}),
                                         gas::slip({0, spatial::Side::High})});
  const auto change = gas::rate(state, grid, closed);
  expect(std::abs(change.density.si_value().sum().item<double>()) < 2e-12,
         "closed slip faces did not conserve total mass");
  expect(std::abs(change.energy.si_value().sum().item<double>()) < 2e-12,
         "closed slip faces did not conserve total energy");

  const auto plane = grid2(8, 6, 0.1, 0.15);
  const Series plane_rho =
      pluss::series::full({8, 6}, 1.1, ScalarType::Float64);
  const Series zero = pluss::series::zeros_like(plane_rho);
  const Series tangent = pluss::series::full_like(plane_rho, 0.7);
  const Series velocity = zero.unsqueeze(0).unite({tangent.unsqueeze(0)}, 0);
  const Series plane_pressure = pluss::series::full_like(plane_rho, 1.0);
  const auto tangential =
      make_state(plane_rho, velocity, plane_pressure, plane);
  const auto slip_rules =
      gas::bounds(plane, {gas::slip({0, spatial::Side::Low}),
                          gas::slip({0, spatial::Side::High}),
                          gas::outlet({1, spatial::Side::Low}),
                          gas::outlet({1, spatial::Side::High})});
  const Series slip_rate =
      gas::rate(tangential, plane, slip_rules).momentum.si_value();
  expect(slip_rate.abs().max().item<double>() < 2e-13,
         "slip walls changed a uniform tangential stream");
}

void test_integration_reverse_and_failures() {
  constexpr std::int64_t n = 20;
  const auto grid = grid1(n, 1.0 / n);
  Series amplitude = pluss::series::full({}, 0.05, ScalarType::Float64);
  amplitude.tracks_slope_(true);
  const Series x = pluss::series::arange(0, n, 1, ScalarType::Float64).add(0.5);
  const Series rho = x.mul(6.283185307179586 / n).sin().mul(amplitude).add(1.1);
  const Series pressure = pluss::series::full_like(rho, 1.0);
  const Series speed = pluss::series::zeros_like(rho).unsqueeze(0);
  const auto state = make_state(rho, speed, pressure, grid);
  const auto rules = gas::bounds(grid, {gas::slip({0, spatial::Side::Low}),
                                        gas::slip({0, spatial::Side::High})});

  gas::RunOptions run;
  run.step.rule = gas::Rule::Rk2;
  run.step.recon = gas::Recon::Minmod;
  run.max_step = seconds(0.003);
  const auto path =
      gas::evolve_to(state, grid, rules, seconds(0.0), seconds(0.006), {}, run);
  expect(path.steps >= 2 && path.flux_evaluations == path.steps * 2 &&
             std::abs(path.trajectory.coordinate()
                          .si_value()
                          .select(0, -1)
                          .item<double>() -
                      0.006) < 1e-15,
         "bounded adaptive evolution lost exact-stop or stage accounting");
  const auto end = final_state(path, grid);
  end.energy().si_value().square().sum().reverse();
  expect(amplitude.has_sensitivity() &&
             std::isfinite(amplitude.sensitivity().item<double>()),
         "bounded gas evolution severed reverse sensitivity");

  expect_failure(
      [&] { (void)gas::bounds(grid, {gas::slip({0, spatial::Side::Low})}); },
      "an incomplete boundary set was accepted");

  const auto plane = grid2(5, 4, 0.1, 0.1);
  const Series bad = pluss::series::ones({3}, ScalarType::Float64);
  const auto bad_state = primitive_data(
      bad, pluss::series::make({0.0, 0.0}, ScalarType::Float64), bad);
  const Series plane_rho = pluss::series::ones({5, 4}, ScalarType::Float64);
  const Series plane_velocity =
      pluss::series::zeros({2, 5, 4}, ScalarType::Float64);
  const auto plane_state = make_state(
      plane_rho, plane_velocity, pluss::series::ones_like(plane_rho), plane);
  const auto malformed =
      gas::bounds(plane, {gas::fixed({0, spatial::Side::Low}, bad_state),
                          gas::outlet({0, spatial::Side::High}),
                          gas::outlet({1, spatial::Side::Low}),
                          gas::outlet({1, spatial::Side::High})});
  expect_failure([&] { (void)gas::rate(plane_state, plane, malformed); },
                 "a non-broadcastable boundary profile was accepted");
}

void test_bounded_weno_reconstruction_and_flow() {
  constexpr std::int64_t n = 12;
  const auto grid = grid1(n, 1.0 / n);
  const Series cells = pluss::series::arange(0, n, 1, ScalarType::Float64)
                           .square()
                           .add(1.0 / 12.0);
  const Series low = pluss::series::arange(-3, 0, 1, ScalarType::Float64)
                         .square()
                         .add(1.0 / 12.0);
  const Series high = pluss::series::arange(n, n + 3, 1, ScalarType::Float64)
                          .square()
                          .add(1.0 / 12.0);
  const auto faces =
      gas::boundary_detail::bounded_weno5_faces(cells, low, high, grid, 0);
  const Series exact =
      pluss::series::arange(0, n, 1, ScalarType::Float64).add(0.5).square();
  expect(faces.first.allclose(exact, 2e-12, 2e-12) &&
             faces.second.allclose(exact, 2e-12, 2e-12),
         "bounded WENO5 did not exactly recover a quadratic cell-average "
         "field");

  const auto plane = grid2(7, 6, 0.1, 0.15);
  const Series rho = pluss::series::full({2, 7, 6}, 1.2, ScalarType::Float64);
  const Series velocity =
      pluss::series::zeros({2, 2, 7, 6}, ScalarType::Float64)
          .add(pluss::series::make({0.25, -0.1}).reshape({1, 2, 1, 1}));
  const Series pressure = pluss::series::full_like(rho, 0.9);
  const auto uniform = make_state(rho, velocity, pressure, plane);
  const auto ambient = scalar_primitive(1.2, {0.25, -0.1}, 0.9);
  const auto far_bounds =
      gas::bounds(plane, {gas::far({0, spatial::Side::Low}, ambient),
                          gas::far({0, spatial::Side::High}, ambient),
                          gas::far({1, spatial::Side::Low}, ambient),
                          gas::far({1, spatial::Side::High}, ambient)});
  gas::Options options;
  options.recon = gas::Recon::Weno5;
  options.flux = gas::FluxScheme::Hllc;
  const auto unchanged = gas::rate(uniform, plane, far_bounds, {}, options);
  expect(unchanged.density.si_value().abs().max().item<double>() < 2e-13 &&
             unchanged.momentum.si_value().abs().max().item<double>() < 2e-12 &&
             unchanged.energy.si_value().abs().max().item<double>() < 2e-12,
         "bounded WENO5 did not preserve a batched multidimensional free "
         "stream");

  constexpr std::int64_t shock_cells = 80;
  const auto shock_grid = grid1(shock_cells, 1.0 / shock_cells);
  const Series coordinate =
      pluss::series::arange(0, shock_cells, 1, ScalarType::Float64);
  const Series left = coordinate.lt(shock_cells / 2);
  const Series shock_rho =
      left.where(pluss::series::full_like(coordinate, 1.0),
                 pluss::series::full_like(coordinate, 0.125));
  const Series shock_pressure =
      left.where(pluss::series::full_like(coordinate, 1.0),
                 pluss::series::full_like(coordinate, 0.1));
  const auto shock = make_state(
      shock_rho, pluss::series::zeros({1, shock_cells}, ScalarType::Float64),
      shock_pressure, shock_grid);
  const auto shock_bounds = gas::bounds(
      shock_grid,
      {gas::fixed({0, spatial::Side::Low}, scalar_primitive(1.0, {0.0}, 1.0)),
       gas::fixed({0, spatial::Side::High},
                  scalar_primitive(0.125, {0.0}, 0.1))});
  const double dt =
      gas::step_limit(shock, shock_grid).si_value().item<double>() * 0.05;
  const auto advanced =
      gas::step(shock, shock_grid, shock_bounds, seconds(dt), {}, options)
          .state;
  expect(advanced.density().si_value().gt(0.0).all().item<bool>() &&
             gas::pressure(advanced, shock_grid)
                 .si_value()
                 .gt(0.0)
                 .all()
                 .item<bool>() &&
             advanced.energy().si_value().isfinite().all().item<bool>(),
         "bounded WENO5/HLLC lost admissibility on the Sod discontinuity");

  const auto short_grid = grid1(4, 0.25);
  const Series short_rho = pluss::series::ones({4}, ScalarType::Float64);
  const auto short_state =
      make_state(short_rho, pluss::series::zeros({1, 4}, ScalarType::Float64),
                 pluss::series::ones_like(short_rho), short_grid);
  const auto short_bounds =
      gas::bounds(short_grid, {gas::wall({0, spatial::Side::Low}),
                               gas::wall({0, spatial::Side::High})});
  expect_failure(
      [&] {
        (void)gas::rate(short_state, short_grid, short_bounds, {}, options);
      },
      "bounded WENO5 accepted an axis too short for its formal stencil");

  Series scale = pluss::series::full({}, 0.03, ScalarType::Float64);
  scale.tracks_slope_(true);
  const Series smooth_rho = pluss::series::arange(0, n, 1, ScalarType::Float64)
                                .mul(6.283185307179586 / n)
                                .sin()
                                .mul(scale)
                                .add(1.0);
  const auto smooth =
      make_state(smooth_rho, pluss::series::zeros({1, n}, ScalarType::Float64),
                 pluss::series::ones_like(smooth_rho), grid);
  const auto closed = gas::bounds(grid, {gas::wall({0, spatial::Side::Low}),
                                         gas::wall({0, spatial::Side::High})});
  gas::rate(smooth, grid, closed, {}, options)
      .momentum.si_value()
      .square()
      .sum()
      .reverse();
  expect(scale.has_sensitivity() &&
             std::isfinite(scale.sensitivity().item<double>()),
         "bounded WENO5 severed reverse sensitivity");
}

void test_characteristic_inlet_contracts() {
  constexpr std::int64_t n = 16;
  constexpr double gamma = 1.4;
  constexpr double gas_constant = 287.052874;
  constexpr double mach = 0.2;
  constexpr double total_temperature = 310.0;
  constexpr double total_pressure = 120000.0;
  const double ratio = 1.0 + 0.5 * (gamma - 1.0) * mach * mach;
  const double static_temperature = total_temperature / ratio;
  const double pressure =
      total_pressure / std::pow(ratio, gamma / (gamma - 1.0));
  const double rho = pressure / (gas_constant * static_temperature);
  const double sound = std::sqrt(gamma * gas_constant * static_temperature);
  const double speed = mach * sound;
  const auto grid = grid1(n, 0.05);
  const auto state =
      make_state(pluss::series::full({n}, rho, ScalarType::Float64),
                 pluss::series::full({1, n}, speed, ScalarType::Float64),
                 pluss::series::full({n}, pressure, ScalarType::Float64), grid);
  const auto total = gas::inlet(
      {0, spatial::Side::Low},
      common::SeriesQuantity<common::Temperature>::from_si(
          pluss::series::full({}, total_temperature, ScalarType::Float64)),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::full({}, total_pressure, ScalarType::Float64)),
      pluss::series::make({1.0}, ScalarType::Float64));
  const auto inside = gas::boundary_detail::plane(gas::Access::raw(state), grid,
                                                  0, spatial::Side::Low);
  const auto recovered =
      gas::boundary_detail::exterior(inside, grid, total, {});
  const double recovered_pressure =
      gas::boundary_detail::plane_pressure(recovered, 0, {}).item<double>();
  expect(std::abs(recovered.rho.item<double>() - rho) < 2e-11 &&
             std::abs(recovered.mom.div(recovered.rho).item<double>() - speed) <
                 2e-10 &&
             std::abs(recovered_pressure - pressure) < 2e-7,
         "total-condition inlet did not recover the ideal-gas reference "
         "state");

  const auto flow =
      gas::inlet({0, spatial::Side::Low},
                 common::SeriesQuantity<gas::Density>::from_si(
                     pluss::series::full({}, rho, ScalarType::Float64)),
                 common::SeriesQuantity<common::Speed>::from_si(
                     pluss::series::full({}, speed, ScalarType::Float64)),
                 pluss::series::make({1.0}, ScalarType::Float64));
  const auto flow_recovered =
      gas::boundary_detail::exterior(inside, grid, flow, {});
  expect(
      std::abs(flow_recovered.rho.item<double>() - rho) < 2e-13 &&
          std::abs(flow_recovered.mom.div(flow_recovered.rho).item<double>() -
                   speed) < 2e-13 &&
          std::abs(gas::boundary_detail::plane_pressure(flow_recovered, 0, {})
                       .item<double>() -
                   pressure) < 2e-8,
      "density-speed inlet did not retain mass flow and the outgoing "
      "characteristic");

  const auto duct = gas::bounds(
      grid, {total,
             gas::outlet(
                 {0, spatial::Side::High},
                 common::SeriesQuantity<common::Pressure>::from_si(
                     pluss::series::full({}, pressure, ScalarType::Float64)))});
  gas::Options options;
  options.recon = gas::Recon::Weno5;
  options.flux = gas::FluxScheme::Hllc;
  const auto stationary = gas::rate(state, grid, duct, {}, options);
  expect(stationary.density.si_value().abs().max().item<double>() < 2e-10 &&
             stationary.momentum.si_value().abs().max().item<double>() < 2e-8 &&
             stationary.energy.si_value().abs().max().item<double>() < 2e-5,
         "matching total inlet and pressure outlet disturbed a uniform "
         "subsonic duct");

  const auto outward = gas::inlet(
      {0, spatial::Side::Low},
      common::SeriesQuantity<common::Temperature>::from_si(
          pluss::series::full({}, total_temperature, ScalarType::Float64)),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::full({}, total_pressure, ScalarType::Float64)),
      pluss::series::make({-1.0}, ScalarType::Float64));
  expect_failure(
      [&] { (void)gas::boundary_detail::exterior(inside, grid, outward, {}); },
      "an outward-pointing inlet direction was accepted");
  const auto supersonic_flow =
      gas::inlet({0, spatial::Side::Low},
                 common::SeriesQuantity<gas::Density>::from_si(
                     pluss::series::full({}, rho, ScalarType::Float64)),
                 common::SeriesQuantity<common::Speed>::from_si(
                     pluss::series::full({}, 3.0 * sound, ScalarType::Float64)),
                 pluss::series::make({1.0}, ScalarType::Float64));
  expect_failure(
      [&] {
        (void)gas::boundary_detail::exterior(inside, grid, supersonic_flow, {});
      },
      "a subsonic density-speed inlet accepted incompatible supersonic data");
}

void test_viscous_couette_wall_work() {
  constexpr std::int64_t nx = 5;
  constexpr std::int64_t ny = 24;
  constexpr double dx = 0.2;
  constexpr double dy = 1.0 / ny;
  constexpr double wall_speed = 0.8;
  constexpr double viscosity = 0.06;
  const auto grid = grid2(nx, ny, dx, dy);
  const Series base = pluss::series::zeros({nx, ny}, ScalarType::Float64);
  const Series ux =
      base.add(pluss::series::arange(0, ny, 1, ScalarType::Float64)
                   .add(0.5)
                   .mul(wall_speed / ny)
                   .reshape({1, ny}));
  const Series velocity =
      ux.unsqueeze(0).unite({pluss::series::zeros_like(base).unsqueeze(0)}, 0);
  const auto state = make_state(pluss::series::ones_like(base), velocity,
                                pluss::series::full_like(base, 1.0), grid);
  const auto rules = gas::bounds(
      grid, {gas::outlet({0, spatial::Side::Low}),
             gas::outlet({0, spatial::Side::High}),
             gas::wall({1, spatial::Side::Low}),
             gas::wall({1, spatial::Side::High},
                       common::SeriesQuantity<common::Speed>::from_si(
                           pluss::series::make({wall_speed, 0.0},
                                               ScalarType::Float64)))});
  gas::Options options;
  options.recon = gas::Recon::Cell;
  options.flux = gas::FluxScheme::Hllc;
  options.transport.shear =
      common::ScalarQuantity<gas::DynamicViscosity>::from_si(viscosity);

  const auto raw = gas::Access::raw(state);
  const Series tau = gas::boundary_detail::bounded_stress(raw, grid, rules, {},
                                                          options.transport);
  expect(
      tau.select(0, 0).select(0, 1).allclose(
          pluss::series::full_like(base, viscosity * wall_speed), 2e-13, 2e-13),
      "linear Couette flow did not produce constant mu*dU/dy shear");

  const auto change = gas::rate(state, grid, rules, {}, options);
  const auto viscous_change =
      gas::boundary_detail::visc_rate(raw, grid, rules, {}, options.transport);
  expect(change.density.si_value().abs().max().item<double>() < 2e-13 &&
             viscous_change.mom.abs().max().item<double>() < 2e-12,
         "linear Couette flow did not preserve mass and momentum");
  const double power = change.energy.si_value().sum().item<double>() * dx * dy;
  expect(std::abs(power - viscosity * wall_speed * wall_speed) < 3e-12,
         "moving-wall work was not assembled as conservative tau dot u flux");
}

void test_isothermal_and_prescribed_heat_walls() {
  constexpr std::int64_t n = 32;
  constexpr double dx = 1.0 / n;
  constexpr double conductivity = 0.15;
  const auto grid = grid1(n, dx);
  gas::Model model;
  model.gas_constant =
      common::ScalarQuantity<gas::SpecificGasConstant>::from_si(1.0);
  const Series temperature = pluss::series::arange(0, n, 1, ScalarType::Float64)
                                 .add(0.5)
                                 .mul(100.0 / n)
                                 .add(300.0);
  const auto state =
      make_state(pluss::series::ones_like(temperature),
                 pluss::series::zeros({1, n}, ScalarType::Float64), temperature,
                 grid, model);
  const auto temp = [](double value) {
    return common::SeriesQuantity<common::Temperature>::from_si(
        pluss::series::full({}, value, ScalarType::Float64));
  };
  const auto isothermal = gas::bounds(
      grid, {gas::wall_temp({0, spatial::Side::Low}, temp(300.0)),
             gas::wall_temp({0, spatial::Side::High}, temp(400.0))});
  gas::Transport heat;
  heat.heat =
      common::ScalarQuantity<gas::ThermalConductivity>::from_si(conductivity);
  const auto viscous = gas::boundary_detail::visc_rate(
      gas::Access::raw(state), grid, isothermal, model, heat);
  expect(viscous.energy.abs().max().item<double>() < 2e-10,
         "a linear temperature field was not an isothermal-wall conduction "
         "equilibrium");
  const auto low_inside = gas::boundary_detail::plane(
      gas::Access::raw(state), grid, 0, spatial::Side::Low);
  const auto high_inside = gas::boundary_detail::plane(
      gas::Access::raw(state), grid, 0, spatial::Side::High);
  expect(gas::boundary_detail::boundary_conduction(
             low_inside, grid, isothermal.at(0, spatial::Side::Low), model,
             pluss::series::full({}, conductivity, ScalarType::Float64))
                 .allclose(pluss::series::full({}, conductivity * 100.0,
                                               ScalarType::Float64),
                           2e-13, 2e-13) &&
             gas::boundary_detail::boundary_conduction(
                 high_inside, grid, isothermal.at(0, spatial::Side::High),
                 model,
                 pluss::series::full({}, conductivity, ScalarType::Float64))
                 .allclose(pluss::series::full({}, conductivity * 100.0,
                                               ScalarType::Float64),
                           2e-13, 2e-13),
         "isothermal walls used the wrong half-cell thermal gradient");

  const auto flux = [](double value) {
    return common::SeriesQuantity<gas::HeatFlux>::from_si(
        pluss::series::full({}, value, ScalarType::Float64));
  };
  const auto imposed =
      gas::bounds(grid, {gas::wall_heat({0, spatial::Side::Low}, flux(2.0)),
                         gas::wall_heat({0, spatial::Side::High}, flux(3.0))});
  const auto uniform = make_state(
      pluss::series::ones({n}, ScalarType::Float64),
      pluss::series::zeros({1, n}, ScalarType::Float64),
      pluss::series::full({n}, 350.0, ScalarType::Float64), grid, model);
  gas::Options options;
  options.recon = gas::Recon::Cell;
  options.transport = heat;
  const double energy_change = gas::rate(uniform, grid, imposed, model, options)
                                   .energy.si_value()
                                   .sum()
                                   .item<double>() *
                               dx;
  expect(std::abs(energy_change + 5.0) < 2e-12,
         "prescribed outward heat flux did not close the global energy "
         "balance");

  const auto adiabatic =
      gas::bounds(grid, {gas::wall({0, spatial::Side::Low}),
                         gas::wall({0, spatial::Side::High})});
  const double closed_change = gas::rate(state, grid, adiabatic, model, options)
                                   .energy.si_value()
                                   .sum()
                                   .item<double>() *
                               dx;
  expect(std::abs(closed_change) < 2e-12,
         "adiabatic walls leaked net conductive energy");
}

void test_wall_failures_and_reverse() {
  constexpr std::int64_t n = 20;
  const auto grid = grid1(n, 1.0 / n);
  Series speed = pluss::series::full({}, 0.1, ScalarType::Float64);
  speed.tracks_slope_(true);
  const Series rho = pluss::series::ones({n}, ScalarType::Float64);
  const auto state =
      make_state(rho, pluss::series::zeros({1, n}, ScalarType::Float64),
                 pluss::series::full_like(rho, 1.0), grid);
  const auto moving = gas::bounds(
      grid, {gas::wall({0, spatial::Side::Low}),
             gas::wall({0, spatial::Side::High},
                       common::SeriesQuantity<common::Speed>::from_si(
                           speed.unsqueeze(0)))});
  gas::Options options;
  options.transport.shear =
      common::ScalarQuantity<gas::DynamicViscosity>::from_si(0.02);
  expect_failure([&] { (void)gas::rate(state, grid, moving, {}, options); },
                 "a normal moving-wall speed was accepted without ALE");

  const auto bad_temp = gas::bounds(
      grid,
      {gas::wall_temp({0, spatial::Side::Low},
                      common::SeriesQuantity<common::Temperature>::from_si(
                          pluss::series::full({}, -1.0, ScalarType::Float64))),
       gas::wall({0, spatial::Side::High})});
  options.transport.heat =
      common::ScalarQuantity<gas::ThermalConductivity>::from_si(0.1);
  expect_failure([&] { (void)gas::rate(state, grid, bad_temp, {}, options); },
                 "a negative wall temperature was accepted");

  const auto plane = grid2(4, n, 0.25, 1.0 / n);
  const Series base = pluss::series::ones({4, n}, ScalarType::Float64);
  const auto tracked =
      make_state(base,
                 base.mul(speed).unsqueeze(0).unite(
                     {pluss::series::zeros_like(base).unsqueeze(0)}, 0),
                 pluss::series::full_like(base, 1.0), plane);
  const auto walls = gas::bounds(plane, {gas::outlet({0, spatial::Side::Low}),
                                         gas::outlet({0, spatial::Side::High}),
                                         gas::wall({1, spatial::Side::Low}),
                                         gas::wall({1, spatial::Side::High})});
  gas::rate(tracked, plane, walls, {}, options)
      .energy.si_value()
      .square()
      .sum()
      .reverse();
  expect(speed.has_sensitivity() &&
             std::isfinite(speed.sensitivity().item<double>()),
         "bounded viscous wall assembly severed reverse sensitivity");
}

void test_bounded_sutherland_transport() {
  constexpr std::int64_t n = 28;
  constexpr double dx = 1.0 / n;
  const auto grid = grid1(n, dx);
  gas::Model model;
  Series amplitude = pluss::series::full({}, 7.0, ScalarType::Float64);
  amplitude.tracks_slope_(true);
  const Series temperature = pluss::series::arange(0, n, 1, ScalarType::Float64)
                                 .add(0.5)
                                 .mul(6.283185307179586 / n)
                                 .sin()
                                 .mul(amplitude)
                                 .add(300.0);
  const Series rho = pluss::series::full_like(temperature, 1.1);
  const auto state = make_state(
      rho, pluss::series::zeros({1, n}, ScalarType::Float64),
      temperature.mul(rho).mul(model.gas_constant.si_value()), grid, model);
  const auto wall_temperature = [](double value) {
    return common::SeriesQuantity<common::Temperature>::from_si(
        pluss::series::full({}, value, ScalarType::Float64));
  };
  const auto rules = gas::bounds(
      grid,
      {gas::wall_temp({0, spatial::Side::Low}, wall_temperature(290.0)),
       gas::wall_temp({0, spatial::Side::High}, wall_temperature(315.0))});
  gas::Options options;
  options.rule = gas::Rule::Rk2;
  options.recon = gas::Recon::Mc;
  options.sutherland = gas::SutherlandLaw{};

  const auto field = gas::transport(state, grid, model, *options.sutherland);
  expect(field.shear.si_value().sizes() == rho.sizes() &&
             field.heat.si_value().sizes() == rho.sizes(),
         "bounded Sutherland transport lost the state layout");
  const auto low_inside = gas::boundary_detail::plane(
      gas::Access::raw(state), grid, 0, spatial::Side::Low);
  const double cell_temperature = temperature.select(0, 0).item<double>();
  const auto mu = [](double value) {
    return 1.716e-5 * std::pow(value / 273.15, 1.5) * (273.15 + 110.4) /
           (value + 110.4);
  };
  const double cp =
      model.gamma * model.gas_constant.si_value() / (model.gamma - 1.0);
  const double cell_k = mu(cell_temperature) * cp / 0.71;
  const double wall_k = mu(290.0) * cp / 0.71;
  const double face_k = 2.0 * cell_k * wall_k / (cell_k + wall_k);
  const double expected = -2.0 * face_k * (290.0 - cell_temperature) / dx;
  const double actual =
      gas::boundary_detail::boundary_conduction(
          low_inside, grid, rules.at(0, spatial::Side::Low), model,
          field.heat.si_value().select(0, 0), options.sutherland)
          .item<double>();
  expect(std::abs(actual - expected) < 2e-12,
         "bounded Sutherland wall conduction did not use harmonic face "
         "conductivity and the half-cell gradient");

  const double limit = gas::step_limit(state, grid, model, *options.sutherland,
                                       options.courant, options.diffusion)
                           .si_value()
                           .item<double>();
  expect(limit > 0.0 && std::isfinite(limit),
         "bounded Sutherland stability policy produced no finite step");
  const auto advanced =
      gas::step(state, grid, rules, seconds(limit * 0.01), model, options);
  advanced.state.energy().si_value().square().sum().reverse();
  expect(advanced.flux_evaluations == 2 && amplitude.has_sensitivity() &&
             std::isfinite(amplitude.sensitivity().item<double>()),
         "bounded Sutherland SSP stepping severed stage accounting or "
         "reverse sensitivity");
}

void test_characteristic_far_field_and_outlet() {
  constexpr std::int64_t n = 8;
  constexpr double gamma = 1.4;
  const auto grid = grid1(n, 0.1);
  const auto uniform = [&](double rho, double speed, double pressure) {
    const Series density = pluss::series::full({n}, rho, ScalarType::Float64);
    return make_state(density,
                      pluss::series::full({1, n}, speed, ScalarType::Float64),
                      pluss::series::full_like(density, pressure), grid);
  };
  const auto inside_state = uniform(1.0, 0.2, 1.0);
  const auto inside = gas::boundary_detail::plane(
      gas::Access::raw(inside_state), grid, 0, spatial::Side::High);
  const auto ambient_data = scalar_primitive(0.8, {-0.05}, 0.7);
  const auto far_rule = gas::far({0, spatial::Side::High}, ambient_data);
  const auto exterior =
      gas::boundary_detail::exterior(inside, grid, far_rule, {});
  const Series exterior_pressure =
      gas::boundary_detail::plane_pressure(exterior, 0, {});
  const double exterior_rho = exterior.rho.item<double>();
  const double exterior_u = exterior.mom.div(exterior.rho).item<double>();
  const double exterior_p = exterior_pressure.item<double>();
  const double exterior_c = std::sqrt(gamma * exterior_p / exterior_rho);
  const double inside_plus = 0.2 + 2.0 * std::sqrt(gamma) / (gamma - 1.0);
  const double ambient_minus =
      -0.05 - 2.0 * std::sqrt(gamma * 0.7 / 0.8) / (gamma - 1.0);
  expect(std::abs(exterior_u + 2.0 * exterior_c / (gamma - 1.0) - inside_plus) <
                 2e-13 &&
             std::abs(exterior_u - 2.0 * exterior_c / (gamma - 1.0) -
                      ambient_minus) < 2e-13 &&
             std::abs(exterior_p / std::pow(exterior_rho, gamma) - 1.0) < 2e-13,
         "subsonic far field did not preserve acoustic invariants and "
         "outgoing entropy");

  const auto supersonic_out = uniform(1.0, 3.0, 1.0);
  const auto sup_out_plane = gas::boundary_detail::plane(
      gas::Access::raw(supersonic_out), grid, 0, spatial::Side::High);
  const auto sup_out =
      gas::boundary_detail::exterior(sup_out_plane, grid, far_rule, {});
  expect(sup_out.rho.equal(sup_out_plane.rho) &&
             sup_out.mom.equal(sup_out_plane.mom) &&
             sup_out.energy.equal(sup_out_plane.energy),
         "supersonic outflow did not ignore the ambient state exactly");

  const auto supersonic_in = uniform(1.0, -3.0, 1.0);
  const auto sup_in_plane = gas::boundary_detail::plane(
      gas::Access::raw(supersonic_in), grid, 0, spatial::Side::High);
  const auto sup_in =
      gas::boundary_detail::exterior(sup_in_plane, grid, far_rule, {});
  expect(std::abs(sup_in.rho.item<double>() - 0.8) < 1e-14 &&
             std::abs(sup_in.mom.item<double>() - (-0.04)) < 1e-14 &&
             std::abs(gas::boundary_detail::plane_pressure(sup_in, 0, {})
                          .item<double>() -
                      0.7) < 1e-14,
         "supersonic inflow did not use the complete ambient state");

  const auto target = common::SeriesQuantity<common::Pressure>::from_si(
      pluss::series::full({}, 0.8, ScalarType::Float64));
  const auto outlet_rule = gas::outlet({0, spatial::Side::High}, target);
  const auto outlet =
      gas::boundary_detail::exterior(inside, grid, outlet_rule, {});
  const double outlet_rho = outlet.rho.item<double>();
  const double outlet_u = outlet.mom.div(outlet.rho).item<double>();
  const double outlet_p =
      gas::boundary_detail::plane_pressure(outlet, 0, {}).item<double>();
  const double outlet_c = std::sqrt(gamma * outlet_p / outlet_rho);
  expect(std::abs(outlet_p - 0.8) < 1e-14 &&
             std::abs(outlet_u + 2.0 * outlet_c / (gamma - 1.0) - inside_plus) <
                 2e-13,
         "subsonic pressure outlet did not retain the outgoing invariant");

  const auto backflow = uniform(1.0, -0.1, 1.0);
  const auto backflow_plane = gas::boundary_detail::plane(
      gas::Access::raw(backflow), grid, 0, spatial::Side::High);
  expect_failure(
      [&] {
        (void)gas::boundary_detail::exterior(backflow_plane, grid, outlet_rule,
                                             {});
      },
      "a pressure outlet silently accepted backflow");
}

void test_blended_and_timed_bounds() {
  constexpr std::int64_t nx = 12;
  constexpr std::int64_t ny = 8;
  const auto plane = grid2(nx, ny, 1.0 / nx, 1.0 / ny);
  const Series rho = pluss::series::ones({nx, ny}, ScalarType::Float64);
  const Series u = pluss::series::full_like(rho, 0.2);
  const Series v = pluss::series::full_like(rho, -0.1);
  const auto state = make_state(
      rho, u.unsqueeze(0).unite({v.unsqueeze(0)}, 0),
      pluss::series::ones_like(rho), plane);
  const Series lane =
      pluss::series::arange(0, nx, 1, ScalarType::Float64);
  const Series mask = lane.lt(nx / 2);
  const auto mixed = gas::blend(
      mask,
      gas::fixed({1, spatial::Side::Low},
                 scalar_primitive(2.0, {0.4, 0.0}, 1.5)),
      gas::slip({1, spatial::Side::Low}));
  const auto inside = gas::boundary_detail::plane(
      gas::Access::raw(state), plane, 1, spatial::Side::Low);
  const auto outside =
      gas::boundary_detail::exterior(inside, plane, mixed, {});
  expect(outside.rho.masked_select(mask).eq(2.0).all().item<bool>() &&
             outside.rho.masked_select(mask.logical_not())
                 .eq(1.0)
                 .all()
                 .item<bool>() &&
             outside.mom.select(0, 1)
                 .masked_select(mask.logical_not())
                 .eq(0.1)
                 .all()
                 .item<bool>(),
         "blended boundary did not select fixed and reflected lanes");
  const auto rules = gas::bounds(
      plane, {gas::outlet({0, spatial::Side::Low}),
              gas::outlet({0, spatial::Side::High}), mixed,
              gas::slip({1, spatial::Side::High})});
  gas::Options weno;
  weno.recon = gas::Recon::Weno5;
  weno.flux = gas::FluxScheme::Hllc;
  const auto change = gas::rate(state, plane, rules, {}, weno);
  expect(change.density.si_value().isfinite().all().item<bool>() &&
             change.momentum.si_value().isfinite().all().item<bool>() &&
             change.energy.si_value().isfinite().all().item<bool>(),
         "blended WENO ghost bands produced a non-finite rate");

  const auto line = grid1(16, 1.0 / 16.0);
  const Series line_rho =
      pluss::series::ones({16}, ScalarType::Float64);
  const auto uniform = make_state(
      line_rho, pluss::series::zeros({1, 16}, ScalarType::Float64), line_rho,
      line);
  const auto fixed_state = scalar_primitive(1.0, {0.0}, 1.0);
  std::vector<double> times;
  gas::BoundFn timed = [&](common::ScalarQuantity<common::Time> time) {
    times.push_back(time.si_value());
    return fixed_ends(line, fixed_state);
  };
  gas::Options rk3;
  rk3.rule = gas::Rule::Rk3;
  rk3.recon = gas::Recon::Cell;
  constexpr double start = 2.0;
  constexpr double width = 1e-3;
  const auto advanced = gas::step(uniform, line, timed, seconds(start),
                                  seconds(width), {}, rk3);
  expect(times.size() == 3 && std::abs(times[0] - start) < 1e-15 &&
             std::abs(times[1] - (start + width)) < 1e-15 &&
             std::abs(times[2] - (start + 0.5 * width)) < 1e-15 &&
             advanced.state.density().si_value().equal(line_rho),
         "timed boundary did not observe the SSP-RK3 abscissae exactly");
}

} // namespace

int main() {
  try {
    test_fixed_state_and_pressure_outlet();
    test_slip_conservation_and_tangential_flow();
    test_integration_reverse_and_failures();
    test_characteristic_far_field_and_outlet();
    test_viscous_couette_wall_work();
    test_isothermal_and_prescribed_heat_walls();
    test_wall_failures_and_reverse();
    test_bounded_sutherland_transport();
    test_bounded_weno_reconstruction_and_flow();
    test_characteristic_inlet_contracts();
    test_blended_and_timed_bounds();
    std::cout << "NatSci bounded compressible-flow checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
