#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace chemistry = pluss::subsp::natsci::chemistry;
namespace gas = pluss::subsp::natsci::physics::fluids::gas;
namespace reacting = gas::reacting;
namespace spatial = common::spatial;
using pluss::ScalarType;
using pluss::Series;

namespace {

constexpr auto F64 = ScalarType::Float64;

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_failure(const std::function<void()> &call,
                    const std::string &message) {
  try {
    call();
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
      {cells}, {common::ScalarQuantity<common::Length>::from_si(1.0 / cells)});
}

chemistry::thermo::SpeciesData species(const char *name, double mass,
                                       double formation_temperature,
                                       const char *element) {
  chemistry::thermo::SpeciesData value;
  value.name = name;
  value.composition = {{element, 1.0}};
  value.molar_mass = mass;
  value.t_min = 200.0;
  value.t_mid = 1000.0;
  value.t_max = 5000.0;
  value.low = {3.5, 0.0, 0.0, 0.0, 0.0, formation_temperature, 0.0};
  value.high = value.low;
  return value;
}

chemistry::kinetics::Mechanism mechanism(bool balanced = true) {
  std::vector<chemistry::thermo::SpeciesData> data{
      species("F", 0.002, 0.0, "F"), species("O", 0.032, 0.0, "O"),
      species("P", balanced ? 0.034 : 0.035, -18000.0, "P")};
  chemistry::kinetics::ReactionSpec reaction;
  reaction.reactants = {{"F", 1.0}, {"O", 1.0}};
  reaction.products = {{"P", 1.0}};
  reaction.a = 20.0;
  reaction.reversible = false;
  return {data, {reaction}, pluss::series::make({300.0}).to(F64)};
}

reacting::State
initial_state(const spatial::PeriodicGrid &grid, const reacting::Model &model,
              Series temperature = pluss::series::full({6}, 1200.0, F64)) {
  const Series density = pluss::series::full({6}, 0.7, F64);
  const Series speed = pluss::series::full({1, 6}, 12.0, F64);
  const Series fractions = pluss::series::make({0.05, 0.80, 0.15}, {3, 1})
                               .to(F64)
                               .mul(pluss::series::ones({1, 6}, F64));
  return reacting::primitive(
      common::SeriesQuantity<gas::Density>::from_si(density),
      common::SeriesQuantity<common::Speed>::from_si(speed),
      common::SeriesQuantity<common::Temperature>::from_si(
          std::move(temperature)),
      fractions, grid, model);
}

void test_primitive_and_source_contract() {
  const auto grid = grid1(6);
  const reacting::Model model(mechanism());
  const auto state = initial_state(grid, model);

  expect(reacting::temperature(state, grid, model)
             .si_value()
             .allclose(pluss::series::full({6}, 1200.0, F64), 2e-11, 2e-11),
         "primitive construction did not recover its temperature");
  expect(reacting::velocity(state, grid)
             .si_value()
             .allclose(pluss::series::full({1, 6}, 12.0, F64), 2e-12, 2e-12),
         "primitive construction did not recover velocity");
  expect(reacting::pressure(state, grid, model)
                 .si_value()
                 .gt(0.0)
                 .all()
                 .item<bool>() &&
             reacting::sound(state, grid, model)
                 .si_value()
                 .gt(0.0)
                 .all()
                 .item<bool>(),
         "reacting primitive state did not have positive pressure and sound "
         "speed");

  const auto source = reacting::source_rate(state, grid, model);
  expect(source.density.si_value().eq(0.0).all().item<bool>() &&
             source.momentum.si_value().eq(0.0).all().item<bool>() &&
             source.energy.si_value().eq(0.0).all().item<bool>(),
         "homogeneous chemistry changed a non-species conserved rate");
  expect(source.partial_density.si_value()
                 .sum(0, false)
                 .abs()
                 .max()
                 .item<double>() < 1e-12,
         "finite-rate source did not conserve mass cell-wise");
}

void test_stiff_reaction_projection() {
  const auto grid = grid1(6);
  const reacting::Model model(mechanism());
  const auto initial = initial_state(grid, model);
  chemistry::reactor::stiff::Options options;
  options.rtol = 1e-7;
  options.atol = 1e-14;
  const auto result =
      reacting::react(initial, grid, seconds(2e-4), model, options);

  expect(result.steps > 0 && result.rhs_evaluations > 0,
         "stiff source step reported no integration work");
  expect(
      result.state.density().si_value().equal(initial.density().si_value()) &&
          result.state.momentum().si_value().equal(
              initial.momentum().si_value()) &&
          result.state.energy().si_value().equal(initial.energy().si_value()),
      "reaction step changed rho, momentum, or total energy");
  expect(result.state.partial_density().si_value().sum(0, false).equal(
             result.state.density().si_value()),
         "projected species densities do not sum exactly to density");

  const Series before = reacting::mass_fractions(initial, grid);
  const Series after = reacting::mass_fractions(result.state, grid);
  expect(after.select(0, 0).lt(before.select(0, 0)).all().item<bool>() &&
             after.select(0, 2).gt(before.select(0, 2)).all().item<bool>(),
         "finite-rate source did not consume fuel and form product");
  expect(reacting::temperature(result.state, grid, model)
             .si_value()
             .gt(1200.0)
             .all()
             .item<bool>(),
         "exothermic chemistry did not raise temperature at conserved energy");
}

void test_batches_reverse_and_failures() {
  const auto grid = grid1(6);
  const reacting::Model model(mechanism());
  Series temperature = pluss::series::full({6}, 1250.0, F64);
  temperature.tracks_slope_(true);
  const auto state = initial_state(grid, model, temperature);
  const Series source =
      reacting::source_rate(state, grid, model).partial_density.si_value();
  source.square().sum().reverse();
  expect(temperature.has_sensitivity() &&
             temperature.sensitivity().isfinite().all().item<bool>(),
         "reacting source severed reverse sensitivity to temperature");

  expect_failure([&] { (void)reacting::Model(mechanism(false)); },
                 "mass-unbalanced mechanism was accepted");
  expect_failure(
      [&] { (void)reacting::react(state, grid, seconds(0.0), model); },
      "zero reaction width was accepted");
  expect_failure(
      [&] {
        const Series bad = reacting::mass_fractions(state, grid).mul(0.5);
        (void)reacting::primitive(
            state.density(), reacting::velocity(state, grid),
            reacting::temperature(state, grid, model), bad, grid, model);
      },
      "non-normalized reacting composition was accepted");
}

} // namespace

int main() {
  try {
    test_primitive_and_source_contract();
    test_stiff_reaction_projection();
    test_batches_reverse_and_failures();
    std::cout << "NatSci compressible reacting checks passed\n";
    return EXIT_SUCCESS;
  } catch (const std::exception &error) {
    std::cerr << "compressible_reacting_test: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
