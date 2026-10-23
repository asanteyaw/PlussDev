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

spatial::PeriodicGrid grid1(std::int64_t cells, double width) {
  return spatial::make_periodic_grid(
      {cells}, {common::ScalarQuantity<common::Length>::from_si(width)});
}

spatial::PeriodicGrid grid3(std::int64_t nx, std::int64_t ny, std::int64_t nz,
                            double dx, double dy, double dz) {
  return spatial::make_periodic_grid(
      {nx, ny, nz}, {common::ScalarQuantity<common::Length>::from_si(dx),
                     common::ScalarQuantity<common::Length>::from_si(dy),
                     common::ScalarQuantity<common::Length>::from_si(dz)});
}

gas::State primitive(const Series &rho, const Series &velocity,
                     const Series &pressure, const spatial::PeriodicGrid &grid,
                     const gas::Model &model = {}) {
  const std::int64_t component = rho.dim() - grid.dim();
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series kinetic =
      velocity.square().sum(component, false).mul(rho).mul(0.5);
  const Series energy = pressure.div(model.gamma - 1.0).add(kinetic);
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(energy),
                   grid, model);
}

gas::Transport transport(double shear, double bulk, double heat) {
  gas::Transport value;
  value.shear = common::ScalarQuantity<gas::DynamicViscosity>::from_si(shear);
  value.bulk = common::ScalarQuantity<gas::DynamicViscosity>::from_si(bulk);
  value.heat = common::ScalarQuantity<gas::ThermalConductivity>::from_si(heat);
  return value;
}

gas::State add_rate(const gas::State &state, const gas::Rate &rate, double dt,
                    const spatial::PeriodicGrid &grid,
                    const gas::Model &model = {}) {
  return gas::make(
      common::SeriesQuantity<gas::Density>::from_si(
          state.density().si_value().add(rate.density.si_value().mul(dt))),
      common::SeriesQuantity<gas::MomDensity>::from_si(
          state.momentum().si_value().add(rate.momentum.si_value().mul(dt))),
      common::SeriesQuantity<gas::EnergyDensity>::from_si(
          state.energy().si_value().add(rate.energy.si_value().mul(dt))),
      grid, model);
}

gas::State blend(const gas::State &left, const gas::State &right,
                 double left_weight, double right_weight,
                 const spatial::PeriodicGrid &grid,
                 const gas::Model &model = {}) {
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(
                       left.density()
                           .si_value()
                           .mul(left_weight)
                           .add(right.density().si_value().mul(right_weight))),
                   common::SeriesQuantity<gas::MomDensity>::from_si(
                       left.momentum()
                           .si_value()
                           .mul(left_weight)
                           .add(right.momentum().si_value().mul(right_weight))),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(
                       left.energy()
                           .si_value()
                           .mul(left_weight)
                           .add(right.energy().si_value().mul(right_weight))),
                   grid, model);
}

Series wave(std::int64_t cells, double width = 1.0) {
  const double dx = width / static_cast<double>(cells);
  return pluss::series::arange(0, cells, 1, ScalarType::Float64)
      .add(0.5)
      .mul(dx * 6.283185307179586)
      .sin();
}

void test_zero_transport_is_exact_euler() {
  constexpr std::int64_t n = 32;
  const auto grid = grid1(n, 1.0 / n);
  const Series x = wave(n);
  const Series rho = x.mul(0.1).add(1.1);
  const Series speed = x.roll(3, 0).mul(0.2).add(0.15).unsqueeze(0);
  const Series pressure = x.roll(-4, 0).mul(0.08).add(1.0);
  const auto state = primitive(rho, speed, pressure, grid);

  gas::Options euler;
  euler.rule = gas::Rule::Rk2;
  euler.recon = gas::Recon::Minmod;
  gas::Options zero = euler;
  zero.transport = transport(0.0, 0.0, 0.0);

  const auto left = gas::rate(state, grid, {}, euler);
  const auto right = gas::rate(state, grid, {}, zero);
  expect(left.density.si_value().equal(right.density.si_value()) &&
             left.momentum.si_value().equal(right.momentum.si_value()) &&
             left.energy.si_value().equal(right.energy.si_value()),
         "zero transport did not recover the Euler rate exactly");

  const double dt =
      gas::step_limit(state, grid).si_value().item<double>() * 0.1;
  const auto a = gas::step(state, grid, seconds(dt), {}, euler).state;
  const auto b = gas::step(state, grid, seconds(dt), {}, zero).state;
  expect(a.density().si_value().equal(b.density().si_value()) &&
             a.momentum().si_value().equal(b.momentum().si_value()) &&
             a.energy().si_value().equal(b.energy().si_value()),
         "zero transport did not recover the Euler step exactly");
}

void test_manufactured_viscous_momentum() {
  constexpr std::int64_t n = 48;
  constexpr double dx = 1.0 / n;
  constexpr double mu = 0.06;
  constexpr double bulk = 0.02;
  const auto grid = grid1(n, dx);
  const Series u = wave(n).mul(0.3);
  const Series rho = pluss::series::full_like(u, 1.4);
  const Series pressure = pluss::series::full_like(u, 2.0);
  const auto state = primitive(rho, u.unsqueeze(0), pressure, grid);

  gas::Options inviscid;
  inviscid.recon = gas::Recon::Cell;
  gas::Options viscous = inviscid;
  viscous.transport = transport(mu, bulk, 0.0);
  const Series actual =
      gas::rate(state, grid, {}, viscous)
          .momentum.si_value()
          .sub(gas::rate(state, grid, {}, inviscid).momentum.si_value())
          .select(0, 0);
  const Series expected =
      spatial::wide_laplacian(u, grid).mul((4.0 / 3.0) * mu + bulk);
  expect(actual.allclose(expected, 2e-12, 2e-12),
         "compressible viscous momentum has the wrong stress divergence");

  const auto tau = gas::stress(state, grid, viscous.transport).si_value();
  const Series expected_tau =
      spatial::partial(u, grid, 0).mul((4.0 / 3.0) * mu + bulk);
  expect(tau.select(0, 0).select(0, 0).allclose(expected_tau, 2e-13, 2e-13),
         "compressible Newtonian stress has the wrong Stokes correction");
}

void test_manufactured_heat_conduction_and_batches() {
  constexpr std::int64_t n = 40;
  constexpr double dx = 1.0 / n;
  constexpr double conductivity = 0.12;
  const auto grid = grid1(n, dx);
  gas::Model model;
  model.gas_constant =
      common::ScalarQuantity<gas::SpecificGasConstant>::from_si(2.0);

  const Series shape = wave(n);
  const Series temp0 = shape.mul(0.15).add(3.0);
  const Series temp1 = shape.roll(5, 0).mul(-0.1).add(4.0);
  const Series temperature = temp0.unsqueeze(0).unite({temp1.unsqueeze(0)}, 0);
  const Series rho = pluss::series::full_like(temperature, 1.25);
  const Series pressure = temperature.mul(rho).mul(2.0);
  const Series velocity = pluss::series::zeros({2, 1, n}, ScalarType::Float64);
  const auto state = primitive(rho, velocity, pressure, grid, model);

  gas::Options inviscid;
  inviscid.recon = gas::Recon::Cell;
  gas::Options heated = inviscid;
  heated.transport = transport(0.0, 0.0, conductivity);
  const Series actual =
      gas::rate(state, grid, model, heated)
          .energy.si_value()
          .sub(gas::rate(state, grid, model, inviscid).energy.si_value());
  const Series expected =
      spatial::wide_laplacian(temperature, grid).mul(conductivity);
  expect(actual.allclose(expected, 2e-11, 2e-11),
         "Fourier conduction has the wrong conservative energy rate");
  expect(gas::temperature(state, grid, model)
             .si_value()
             .allclose(temperature, 2e-14, 2e-14),
         "ideal-gas temperature recovery lost batch lanes");
  expect(gas::heat_flux(state, grid, model, heated.transport)
             .si_value()
             .select(1, 0)
             .allclose(spatial::gradient(temperature, grid)
                           .select(1, 0)
                           .mul(-conductivity),
                       2e-14, 2e-14),
         "Fourier heat flux has the wrong sign or batch layout");
}

void test_three_dimensional_shear() {
  constexpr std::int64_t nx = 4;
  constexpr std::int64_t ny = 32;
  constexpr std::int64_t nz = 3;
  constexpr double dy = 1.0 / ny;
  constexpr double mu = 0.075;
  const auto grid = grid3(nx, ny, nz, 0.2, dy, 0.3);
  const Series base = pluss::series::zeros({nx, ny, nz}, ScalarType::Float64);
  const Series ux = base.add(wave(ny).reshape({1, ny, 1}).mul(0.25));
  const Series zero = pluss::series::zeros_like(base);
  const Series velocity =
      ux.unsqueeze(0).unite({zero.unsqueeze(0), zero.unsqueeze(0)}, 0);
  const Series rho = pluss::series::full_like(base, 1.1);
  const Series pressure = pluss::series::full_like(base, 1.3);
  const auto state = primitive(rho, velocity, pressure, grid);

  gas::Options inviscid;
  inviscid.recon = gas::Recon::Cell;
  gas::Options viscous = inviscid;
  viscous.transport = transport(mu, 0.0, 0.0);
  const Series delta =
      gas::rate(state, grid, {}, viscous)
          .momentum.si_value()
          .sub(gas::rate(state, grid, {}, inviscid).momentum.si_value());
  expect(delta.select(0, 0).allclose(spatial::wide_laplacian(ux, grid).mul(mu),
                                     3e-12, 3e-12) &&
             delta.select(0, 1).abs().max().item<double>() < 3e-12 &&
             delta.select(0, 2).abs().max().item<double>() < 3e-12,
         "three-dimensional shear diffusion mixed components or axes");
}

void test_sutherland_transport_values_and_layout() {
  constexpr std::int64_t n = 4;
  const auto grid = grid1(n, 0.25);
  gas::Model model;
  const Series temperature =
      pluss::series::make({200.0, 273.15, 300.0, 1000.0}, ScalarType::Float64);
  const Series rho = pluss::series::ones_like(temperature);
  const Series pressure = temperature.mul(model.gas_constant.si_value());
  const Series velocity = pluss::series::zeros({1, n}, ScalarType::Float64);
  const auto state = primitive(rho, velocity, pressure, grid, model);

  gas::SutherlandLaw law;
  law.bulk_ratio = 0.17;
  const auto field = gas::transport(state, grid, model, law);
  const auto expected_mu = [](double value) {
    constexpr double reference_mu = 1.716e-5;
    constexpr double reference_temperature = 273.15;
    constexpr double sutherland_temperature = 110.4;
    return reference_mu * std::pow(value / reference_temperature, 1.5) *
           (reference_temperature + sutherland_temperature) /
           (value + sutherland_temperature);
  };
  const Series expected =
      pluss::series::make({expected_mu(200.0), expected_mu(273.15),
                           expected_mu(300.0), expected_mu(1000.0)},
                          ScalarType::Float64);
  const double cp =
      model.gamma * model.gas_constant.si_value() / (model.gamma - 1.0);
  expect(field.shear.si_value().allclose(expected, 1e-14, 1e-18),
         "Sutherland viscosity does not match the reference-temperature law");
  expect(field.bulk.si_value().allclose(expected.mul(0.17), 1e-14, 1e-18),
         "Sutherland bulk viscosity did not preserve its configured ratio");
  expect(field.heat.si_value().allclose(expected.mul(cp / law.prandtl), 1e-14,
                                        1e-15),
         "fixed-Prandtl conductivity does not equal mu*cp/Pr");

  const Series batched_temperature =
      temperature.unsqueeze(0).unite({temperature.roll(1, 0).unsqueeze(0)}, 0);
  const Series batched_rho = pluss::series::ones_like(batched_temperature);
  const auto batched = primitive(
      batched_rho, pluss::series::zeros({2, 1, n}, ScalarType::Float64),
      batched_temperature.mul(model.gas_constant.si_value()), grid, model);
  const auto batched_field = gas::transport(batched, grid, model, law);
  expect(batched_field.shear.si_value().sizes() == batched_rho.sizes() &&
             batched_field.heat.si_value().sizes() == batched_rho.sizes(),
         "Sutherland transport did not preserve leading batch lanes");
  expect(batched_field.shear.si_value().select(0, 0).allclose(expected, 1e-14,
                                                              1e-18) &&
             batched_field.shear.si_value().select(0, 1).allclose(
                 expected.roll(1, 0), 1e-14, 1e-18),
         "Sutherland transport mixed independent batch lanes");
}

void test_sutherland_constitutive_and_ssp_paths() {
  constexpr std::int64_t n = 32;
  constexpr double dx = 1.0 / n;
  const auto grid = grid1(n, dx);
  gas::Model model;
  const Series x = wave(n);
  const Series rho = x.mul(0.04).add(1.1);
  const Series temperature = x.roll(3, 0).mul(18.0).add(310.0);
  const Series pressure =
      temperature.mul(rho).mul(model.gas_constant.si_value());
  const Series velocity = x.roll(-2, 0).mul(0.03).add(0.08).unsqueeze(0);
  const auto state = primitive(rho, velocity, pressure, grid, model);

  gas::Options options;
  options.rule = gas::Rule::Rk2;
  options.recon = gas::Recon::Mc;
  options.sutherland = gas::SutherlandLaw{};
  const double dt = gas::limits(state, grid, model, *options.sutherland)
                        .allowed.si_value()
                        .item<double>() *
                    0.05;
  const gas::Rate first_rate = gas::rate(state, grid, model, options);
  const gas::State first = add_rate(state, first_rate, dt, grid, model);
  const gas::Rate second_rate = gas::rate(first, grid, model, options);
  const gas::State second = add_rate(first, second_rate, dt, grid, model);
  const gas::State expected = blend(state, second, 0.5, 0.5, grid, model);
  const gas::State actual =
      gas::step(state, grid, seconds(dt), model, options).state;
  expect(actual.density().si_value().allclose(expected.density().si_value(),
                                              2e-13, 2e-13) &&
             actual.momentum().si_value().allclose(
                 expected.momentum().si_value(), 2e-13, 2e-13) &&
             actual.energy().si_value().allclose(expected.energy().si_value(),
                                                 2e-13, 2e-13),
         "SSP-RK2 did not recompute temperature-dependent transport at its "
         "second stage");

  const Series uniform_temperature = pluss::series::full_like(x, 273.15);
  const auto uniform =
      primitive(pluss::series::full_like(x, 1.2), velocity,
                uniform_temperature.mul(1.2).mul(model.gas_constant.si_value()),
                grid, model);
  gas::SutherlandLaw law;
  const auto field = gas::transport(uniform, grid, model, law);
  const gas::Transport constant = transport(
      law.reference_viscosity.si_value(), 0.0,
      law.reference_viscosity.si_value() * model.gamma *
          model.gas_constant.si_value() / ((model.gamma - 1.0) * law.prandtl));
  expect(gas::stress(uniform, grid, field)
             .si_value()
             .allclose(gas::stress(uniform, grid, constant).si_value(), 2e-14,
                       2e-18),
         "uniform-temperature Sutherland stress did not recover constant "
         "transport");
  expect(gas::heat_flux(uniform, grid, model, field)
                 .si_value()
                 .abs()
                 .max()
                 .item<double>() < 1e-16,
         "uniform temperature produced a nonzero Fourier heat flux");
}

void test_sutherland_reverse_and_failures() {
  constexpr std::int64_t n = 24;
  const auto grid = grid1(n, 1.0 / n);
  gas::Model model;
  Series amplitude = pluss::series::full({}, 9.0, ScalarType::Float64);
  amplitude.tracks_slope_(true);
  const Series temperature = wave(n).mul(amplitude).add(300.0);
  const Series rho = pluss::series::full_like(temperature, 1.05);
  const auto state = primitive(
      rho, pluss::series::zeros({1, n}, ScalarType::Float64),
      temperature.mul(rho).mul(model.gas_constant.si_value()), grid, model);
  gas::Options options;
  options.rule = gas::Rule::Euler;
  options.recon = gas::Recon::Cell;
  options.sutherland = gas::SutherlandLaw{};
  const double dt = gas::limits(state, grid, model, *options.sutherland)
                        .allowed.si_value()
                        .item<double>() *
                    0.02;
  gas::step(state, grid, seconds(dt), model, options)
      .state.energy()
      .si_value()
      .square()
      .sum()
      .reverse();
  expect(amplitude.has_sensitivity() &&
             std::isfinite(amplitude.sensitivity().item<double>()),
         "Sutherland transport severed reverse sensitivity");

  expect_failure(
      [&] {
        gas::SutherlandLaw invalid;
        invalid.prandtl = 0.0;
        (void)gas::transport(state, grid, model, invalid);
      },
      "zero Prandtl number was accepted");
  expect_failure(
      [&] {
        gas::SutherlandLaw invalid;
        invalid.sutherland_temperature =
            common::ScalarQuantity<common::Temperature>::from_si(-1.0);
        (void)gas::transport(state, grid, model, invalid);
      },
      "negative Sutherland temperature was accepted");
  expect_failure(
      [&] {
        gas::Options invalid = options;
        invalid.transport = transport(1e-5, 0.0, 0.0);
        (void)gas::rate(state, grid, model, invalid);
      },
      "constant and Sutherland transport were accepted together");
  expect_failure(
      [&] {
        const Series wrong = pluss::series::zeros({n + 1}, ScalarType::Float64);
        gas::TransportField invalid{
            common::SeriesQuantity<gas::DynamicViscosity>::from_si(wrong),
            common::SeriesQuantity<gas::DynamicViscosity>::from_si(wrong),
            common::SeriesQuantity<gas::ThermalConductivity>::from_si(wrong)};
        (void)gas::stress(state, grid, invalid);
      },
      "a shape-mismatched transport field was accepted");
}

void test_sutherland_float32_contract() {
  constexpr std::int64_t n = 16;
  const auto grid = grid1(n, 1.0 / n);
  gas::Model model;
  const Series temperature =
      pluss::series::full({n}, 300.0, ScalarType::Float32);
  const Series rho = pluss::series::full_like(temperature, 1.1);
  const auto state = primitive(
      rho, pluss::series::zeros({1, n}, ScalarType::Float32),
      temperature.mul(rho).mul(model.gas_constant.si_value()), grid, model);
  const auto field = gas::transport(state, grid, model, gas::SutherlandLaw{});
  expect(field.shear.si_value().scalar_type() == ScalarType::Float32 &&
             field.bulk.si_value().scalar_type() == ScalarType::Float32 &&
             field.heat.si_value().scalar_type() == ScalarType::Float32 &&
             field.shear.si_value().platform() == rho.platform(),
         "Sutherland transport did not preserve Float32 dtype and platform");
  gas::Options options;
  options.sutherland = gas::SutherlandLaw{};
  const auto change = gas::rate(state, grid, model, options);
  expect(change.energy.si_value().scalar_type() == ScalarType::Float32 &&
             change.energy.si_value().isfinite().all().item<bool>(),
         "Float32 Sutherland residual changed dtype or became non-finite");
}

void test_limits_conservation_failures_and_reverse() {
  constexpr std::int64_t n = 36;
  constexpr double dx = 1.0 / n;
  const auto grid = grid1(n, dx);
  const Series x = wave(n);
  const Series rho = pluss::series::full_like(x, 2.0);
  const Series pressure = pluss::series::full_like(x, 1.0);
  Series amplitude = pluss::series::full({}, 0.2, ScalarType::Float64);
  amplitude.tracks_slope_(true);
  const Series speed = x.mul(amplitude).unsqueeze(0);
  const auto state = primitive(rho, speed, pressure, grid);

  gas::Options options;
  options.rule = gas::Rule::Euler;
  options.recon = gas::Recon::Cell;
  options.transport = transport(0.08, 0.03, 0.04);
  const auto report = gas::limits(state, grid, {}, options.transport,
                                  options.courant, options.diffusion);
  const double cv = 287.052874 / 0.4;
  const double max_diff =
      std::max(((4.0 / 3.0) * 0.08 + 0.03) / 2.0, 0.04 / (2.0 * cv));
  const double expected_diff = 0.5 / (max_diff / (dx * dx));
  expect(std::abs(report.diffusion.si_value().item<double>() - expected_diff) <
             2e-15,
         "viscous/thermal stability limit is not the explicit diffusion "
         "bound");
  expect(report.allowed.si_value()
                 .le(report.acoustic.si_value())
                 .all()
                 .item<bool>() &&
             report.allowed.si_value()
                 .le(report.diffusion.si_value())
                 .all()
                 .item<bool>(),
         "combined gas step limit did not select the tighter constraint");

  const auto total = gas::rate(state, grid, {}, options);
  expect(std::abs(total.density.si_value().sum().item<double>()) < 2e-12 &&
             total.momentum.si_value().sum().abs().item<double>() < 2e-12 &&
             std::abs(total.energy.si_value().sum().item<double>()) < 2e-12,
         "periodic Navier-Stokes-Fourier rate is not conservative");

  const double dt = report.allowed.si_value().item<double>() * 0.2;
  const auto advanced = gas::step(state, grid, seconds(dt), {}, options);
  advanced.state.energy().si_value().square().sum().reverse();
  expect(amplitude.has_sensitivity() &&
             std::isfinite(amplitude.sensitivity().item<double>()),
         "viscous compressible stepping severed reverse sensitivity");

  expect_failure(
      [&] {
        gas::Options invalid = options;
        invalid.transport = transport(-0.1, 0.0, 0.0);
        (void)gas::rate(state, grid, {}, invalid);
      },
      "negative shear viscosity was accepted");
  expect_failure(
      [&] {
        gas::Options invalid = options;
        invalid.diffusion = 0.75;
        (void)gas::step(state, grid, seconds(dt), {}, invalid);
      },
      "an unstable explicit diffusion coefficient was accepted");
  expect_failure(
      [&] {
        (void)gas::step(
            state, grid,
            seconds(report.allowed.si_value().item<double>() * 1.01), {},
            options);
      },
      "a step beyond the viscous gas limit was accepted");
}

} // namespace

int main() {
  try {
    test_zero_transport_is_exact_euler();
    test_manufactured_viscous_momentum();
    test_manufactured_heat_conduction_and_batches();
    test_three_dimensional_shear();
    test_sutherland_transport_values_and_layout();
    test_sutherland_constitutive_and_ssp_paths();
    test_sutherland_reverse_and_failures();
    test_sutherland_float32_contract();
    test_limits_conservation_failures_and_reverse();
    std::cout << "NatSci compressible Navier-Stokes-Fourier checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
