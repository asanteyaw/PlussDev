#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace gas = pluss::subsp::natsci::physics::fluids::gas;
namespace species = gas::species;
namespace transport = pluss::subsp::natsci::chemistry::transport;

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

spatial::PeriodicGrid grid1(std::int64_t cells) {
  return spatial::make_periodic_grid(
      {cells}, {common::ScalarQuantity<common::Length>::from_si(
                   1.0 / static_cast<double>(cells))});
}

spatial::PeriodicGrid grid2(std::int64_t nx, std::int64_t ny) {
  return spatial::make_periodic_grid(
      {nx, ny}, {common::ScalarQuantity<common::Length>::from_si(1.0 / nx),
                 common::ScalarQuantity<common::Length>::from_si(1.0 / ny)});
}

gas::State primitive(const Series &rho, const Series &speed,
                     const Series &pressure, const spatial::PeriodicGrid &grid,
                     const gas::Model &model = {}) {
  const std::int64_t component = rho.dim() - grid.dim();
  const Series velocity =
      speed.dim() == rho.dim() ? speed.unsqueeze(component) : speed;
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series energy =
      pressure.div(model.gamma - 1.0)
          .add(velocity.square().sum(component, false).mul(rho).mul(0.5));
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(energy),
                   grid, model);
}

Series binary(const Series &first);
transport::GasTransport binary_transport(const Series &like);

void test_rank_four_batched_two_dimensional_layout() {
  constexpr std::int64_t nx = 8;
  constexpr std::int64_t ny = 6;
  const auto grid = grid2(nx, ny);
  const Series rho = pluss::series::full({2, nx, ny}, 1.0, ScalarType::Float64);
  const Series pressure =
      pluss::series::full({2, nx, ny}, 101325.0, ScalarType::Float64);
  const Series u = pluss::series::full_like(rho, 12.0);
  const Series v = pluss::series::full_like(rho, -4.0);
  const Series velocity = u.unsqueeze(1).unite({v.unsqueeze(1)}, 1);
  const auto flow = primitive(rho, velocity, pressure, grid);

  const Series x =
      pluss::series::arange(0, nx, 1, ScalarType::Float64)
          .add(0.5)
          .div(nx)
          .reshape({nx, 1})
          .add(pluss::series::zeros({nx, ny}, ScalarType::Float64));
  const Series lane0 = x.mul(2.0 * M_PI).sin().mul(0.1).add(0.45);
  const Series lane1 = x.mul(2.0 * M_PI).cos().mul(0.08).add(0.55);
  const Series first = lane0.unsqueeze(0).unite({lane1.unsqueeze(0)}, 0);
  const Series fractions =
      first.unsqueeze(1).unite({first.neg().add(1.0).unsqueeze(1)}, 1);
  const auto state = species::make(flow, fractions, grid);
  auto model = binary_transport(x);
  species::Options options;
  options.transport = &model;
  options.flow.recon = gas::Recon::Minmod;

  const Series flux =
      species::diffusive_flux(state, grid, {}, model).si_value();
  expect(flux.sizes() == std::vector<std::int64_t>({2, 2, 2, nx, ny}) &&
             flux.sum(1, false).abs().max().item<double>() < 2e-18,
         "two-dimensional batched species flux has the wrong layout or net "
         "mass flux");

  const auto change = species::rate(state, grid, {}, options);
  expect(change.partial_density.si_value().sum(1, false).allclose(
             change.flow.density.si_value(), 2e-11, 2e-11) &&
             change.partial_density.si_value()
                     .sum(std::vector<std::int64_t>{2, 3}, false)
                     .abs()
                     .max()
                     .item<double>() < 2e-10,
         "rank-four species rate broke gas coupling or periodic "
         "conservation");
}

Series binary(const Series &first) {
  return first.unsqueeze(-2).unite({first.neg().add(1.0).unsqueeze(-2)}, -2);
}

transport::GasTransport binary_transport(const Series &like) {
  std::vector<transport::SpeciesData> data(2);
  data[0] = {"A", 0, 97.53, 3.621, 0.0, 0.0, 0.0280134};
  data[1] = {"B", 0, 119.80, 3.458, 0.0, 0.0, 0.0319988};
  return transport::GasTransport(data, like);
}

void test_uniform_and_periodic_conservation() {
  constexpr std::int64_t n = 48;
  const auto grid = grid1(n);
  const Series x =
      pluss::series::arange(0, n, 1, ScalarType::Float64).add(0.5).div(n);
  const Series rho = pluss::series::full_like(x, 1.2);
  const Series pressure = pluss::series::full_like(x, 101325.0);
  const Series speed = pluss::series::full_like(x, 35.0);
  const auto flow = primitive(rho, speed, pressure, grid);

  const Series uniform = binary(pluss::series::full_like(x, 0.3));
  const auto uniform_state = species::make(flow, uniform, grid);
  species::Options options;
  options.flow.rule = gas::Rule::Rk3;
  options.flow.recon = gas::Recon::Weno5;
  options.flow.flux = gas::FluxScheme::Hllc;
  const double dt = species::limits(uniform_state, grid, {}, options)
                        .allowed.si_value()
                        .item<double>() *
                    0.2;
  const auto uniform_next =
      species::step(uniform_state, grid, seconds(dt), {}, options);
  expect(species::mass_fractions(uniform_next.state, grid)
             .allclose(uniform, 2e-13, 2e-13),
         "uniform species fractions did not remain exact");
  expect(uniform_next.flux_evaluations == 3,
         "species RK3 did not report three synchronized flux evaluations");

  const auto fixed = species::evolve(uniform_state, grid, seconds(0.0),
                                     seconds(dt), 3, {}, options);
  expect(fixed.steps == 3 && fixed.flux_evaluations == 9 &&
             fixed.trajectory.state()
                     .get<gas::Density>("species_density")
                     .si_value()
                     .sizes() == std::vector<std::int64_t>({4, 2, n}),
         "fixed species evolution did not retain the full composition path");

  species::RunOptions run;
  run.step = options;
  run.max_step = seconds(dt);
  const auto adaptive = species::evolve_to(uniform_state, grid, seconds(0.0),
                                           seconds(2.25 * dt), {}, run);
  expect(adaptive.steps == 3 && std::abs(adaptive.trajectory.coordinate()
                                             .si_value()
                                             .select(0, -1)
                                             .item<double>() -
                                         2.25 * dt) < 1e-15,
         "adaptive species evolution did not terminate exactly at the stop "
         "time");

  const Series first = x.mul(2.0 * M_PI).sin().mul(0.2).add(0.5);
  const auto state = species::make(flow, binary(first), grid);
  const Series before = state.partial_density().si_value().sum(-1, false);
  const auto next = species::step(state, grid, seconds(dt), {}, options).state;
  const Series after = next.partial_density().si_value().sum(-1, false);
  expect(after.allclose(before, 2e-11, 2e-11),
         "periodic species advection did not conserve each species mass");
  expect(next.partial_density().si_value().sum(0, false).allclose(
             next.flow().density().si_value(), 2e-12, 2e-12),
         "advected partial densities no longer sum to gas density");
}

void test_diffusion_flux_limit_and_smoothing() {
  constexpr std::int64_t n = 40;
  const auto grid = grid1(n);
  const Series x =
      pluss::series::arange(0, n, 1, ScalarType::Float64).add(0.5).div(n);
  const Series rho = pluss::series::full_like(x, 1.0);
  const Series pressure = pluss::series::full_like(x, 101325.0);
  const auto flow =
      primitive(rho, pluss::series::zeros_like(x), pressure, grid);
  const Series first = x.mul(2.0 * M_PI).sin().mul(0.18).add(0.5);
  const auto state = species::make(flow, binary(first), grid);
  auto model = binary_transport(x);

  const Series flux =
      species::diffusive_flux(state, grid, {}, model).si_value();
  expect(flux.sum(0, false).abs().max().item<double>() < 2e-18,
         "mixture correction velocity did not produce zero net mass flux");

  species::Options options;
  options.flow.rule = gas::Rule::Euler;
  options.flow.recon = gas::Recon::Cell;
  options.transport = &model;
  const auto report = species::limits(state, grid, {}, options);
  const Series y_last = species::mass_fractions(state, grid).transpose(0, 1);
  const Series x_last = model.mole_fractions(y_last);
  const Series diffusion = model.mixture_diffusion_chemkin(
      gas::temperature(flow, grid).si_value(), x_last, pressure);
  const double expected_limit =
      options.diffusion /
      (diffusion.max().item<double>() * static_cast<double>(n * n));
  expect(report.diffusion.si_value().isfinite().all().item<bool>() &&
             std::abs(report.diffusion.si_value().item<double>() -
                      expected_limit) < expected_limit * 2e-12 &&
             report.allowed.si_value()
                 .le(report.flow.si_value())
                 .all()
                 .item<bool>() &&
             report.allowed.si_value()
                 .le(report.diffusion.si_value())
                 .all()
                 .item<bool>(),
         "species stability report did not combine acoustic and diffusion "
         "limits");

  const double before = first.sub(0.5).square().mean().item<double>();
  const Series species_mass_before =
      state.partial_density().si_value().sum(-1, false);
  const auto next = species::step(
      state, grid, seconds(report.allowed.si_value().item<double>() * 0.15), {},
      options);
  const Series y = species::mass_fractions(next.state, grid).select(0, 0);
  const double after = y.sub(0.5).square().mean().item<double>();
  expect(after < before,
         "mixture diffusion did not reduce composition variance");
  expect(next.state.partial_density().si_value().sum(-1, false).allclose(
             species_mass_before, 2e-12, 2e-12),
         "periodic diffusion did not conserve each species mass");
}

void test_batches_reverse_and_failures() {
  constexpr std::int64_t n = 20;
  const auto grid = grid1(n);
  const Series x =
      pluss::series::arange(0, n, 1, ScalarType::Float64).add(0.5).div(n);
  const Series rho = pluss::series::full({2, n}, 1.1, ScalarType::Float64);
  const Series pressure =
      pluss::series::full({2, n}, 101325.0, ScalarType::Float64);
  const auto flow =
      primitive(rho, pluss::series::zeros_like(rho), pressure, grid);
  Series amplitude = pluss::series::make(0.12);
  amplitude.tracks_slope_(true);
  const Series wave = x.mul(2.0 * M_PI).sin().mul(amplitude).add(0.5);
  const Series first = wave.unsqueeze(0).unite({wave.mul(0.8).unsqueeze(0)}, 0);
  const auto state = species::make(flow, binary(first), grid);
  auto model = binary_transport(x);
  species::Options options;
  options.transport = &model;
  options.flow.rule = gas::Rule::Euler;
  options.flow.recon = gas::Recon::Minmod;
  const double dt = species::limits(state, grid, {}, options)
                        .allowed.si_value()
                        .min()
                        .item<double>() *
                    0.1;
  const auto next = species::step(state, grid, seconds(dt), {}, options).state;
  next.partial_density().si_value().select(1, 0).square().sum().reverse();
  expect(next.partial_density().si_value().sizes() ==
                 std::vector<std::int64_t>({2, 2, n}) &&
             amplitude.has_sensitivity() &&
             std::isfinite(amplitude.sensitivity().item<double>()),
         "batched species stepping changed rank or severed reverse "
         "sensitivity");

  expect_failure(
      [&] {
        const Series lane = pluss::series::full_like(first, 0.2);
        const Series bad = lane.unsqueeze(-2).unite({lane.unsqueeze(-2)}, -2);
        (void)species::make(flow, bad, grid);
      },
      "non-normalized mass fractions were accepted");
  expect_failure(
      [&] {
        const Series bad =
            pluss::series::full({2, n}, -0.1, ScalarType::Float64);
        (void)species::make(flow, bad, grid);
      },
      "negative mass fractions were accepted");
  expect_failure(
      [&] {
        std::vector<transport::SpeciesData> one{
            {"only", 0, 97.53, 3.621, 0.0, 0.0, 0.0280134}};
        transport::GasTransport wrong(one, x);
        (void)species::diffusive_flux(state, grid, {}, wrong);
      },
      "a transport model with the wrong species count was accepted");
}

} // namespace

int main() {
  try {
    test_uniform_and_periodic_conservation();
    test_diffusion_flux_limit_and_smoothing();
    test_rank_four_batched_two_dimensional_layout();
    test_batches_reverse_and_failures();
    std::cout << "compressible species tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
