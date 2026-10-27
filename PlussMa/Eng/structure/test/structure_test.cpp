#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace structure = pluss::subsp::eng::structure;

static_assert(!std::is_default_constructible_v<structure::Truss>);
static_assert(std::is_copy_constructible_v<structure::Truss>);

namespace {

void expect(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}

void close(const pluss::Series &value, const std::vector<double> &expected,
           double tolerance = 1e-9) {
  const auto actual = value.host<double>();
  expect(actual.size() == expected.size(), "result size mismatch");
  for (std::size_t lane = 0; lane < actual.size(); ++lane)
    if (std::abs(actual[lane] - expected[lane]) > tolerance)
      throw std::runtime_error("result mismatch at lane " +
                               std::to_string(lane) + ": got " +
                               std::to_string(actual[lane]) + ", expected " +
                               std::to_string(expected[lane]));
}

void expect_error(const std::function<void()> &call,
                  const std::string &message) {
  try {
    call();
  } catch (const std::invalid_argument &) {
    return;
  } catch (const std::out_of_range &) {
    return;
  } catch (const std::runtime_error &) {
    return;
  }
  throw std::runtime_error(message);
}

structure::Mesh bar_mesh() {
  return {pluss::series::make({{0.0, 0.0}, {2.0, 0.0}})
              .to(pluss::ScalarType::Float64),
          {0},
          {1}};
}

structure::Bars bar_props() {
  return {pluss::series::make({2.0}).to(pluss::ScalarType::Float64),
          pluss::series::make({100.0}).to(pluss::ScalarType::Float64)};
}

structure::Fix bar_fix() { return {{0, 0, 1}, {0, 1, 1}, {}}; }

pluss::Series bar_load(double force) {
  return pluss::series::make({{0.0, 0.0}, {force, 0.0}})
      .to(pluss::ScalarType::Float64);
}

} // namespace

int main() {
  const auto truss = structure::truss(bar_mesh(), bar_props());
  expect(truss.nodes() == 2 && truss.bars() == 1 && truss.dim() == 2,
         "compiled topology metadata is wrong");

  const auto axial = structure::solve(truss, bar_load(10.0), bar_fix());
  close(axial.displacement, {0.0, 0.0, 0.1, 0.0});
  close(axial.reaction, {-10.0, 0.0, 0.0, 0.0});
  close(axial.strain, {0.05});
  close(axial.stress, {5.0});
  close(axial.force, {10.0});
  close(axial.energy, {0.5});
  expect(axial.converged.item<bool>() && !axial.breakdown.item<bool>(),
         "exact bar solve did not converge");

  structure::Fix imposed = bar_fix();
  imposed.node.push_back(1);
  imposed.axis.push_back(0);
  imposed.value =
      pluss::series::make({0.0, 0.0, 0.0, 0.02}).to(pluss::ScalarType::Float64);
  const auto prescribed = structure::solve(
      truss,
      pluss::series::zeros(
          {2, 2}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)),
      imposed);
  close(prescribed.displacement, {0.0, 0.0, 0.02, 0.0});
  close(prescribed.reaction, {-2.0, 0.0, 2.0, 0.0});
  close(prescribed.force, {2.0});
  close(prescribed.energy, {0.02});

  const auto batch_load = pluss::series::make(
      std::vector<double>{0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 20.0, 0.0}, {2, 2, 2});
  const auto batch = structure::solve(truss, batch_load, bar_fix());
  close(batch.displacement, {0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.2, 0.0});
  close(batch.force, {10.0, 20.0});
  close(batch.energy, {0.5, 2.0});
  expect(batch.converged.sizes() == std::vector<std::int64_t>{2},
         "load-case axis was not preserved");

  const auto model_batch = structure::truss(
      bar_mesh(),
      {pluss::series::make(std::vector<double>{2.0, 4.0}, {2, 1}),
       pluss::series::make({100.0}).to(pluss::ScalarType::Float64)});
  const auto model_batch_result =
      structure::solve(model_batch, bar_load(10.0), bar_fix());
  close(model_batch_result.displacement,
        {0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.05, 0.0});
  close(model_batch_result.force, {10.0, 10.0});
  close(model_batch_result.energy, {0.5, 0.25});

  const auto f32_truss = structure::truss(
      {bar_mesh().position.to(pluss::ScalarType::Float32), {0}, {1}},
      {bar_props().area.to(pluss::ScalarType::Float32),
       bar_props().modulus.to(pluss::ScalarType::Float32)});
  const auto f32_result = structure::solve(
      f32_truss, bar_load(10.0).to(pluss::ScalarType::Float32), bar_fix());
  close(f32_result.displacement, {0.0, 0.0, 0.1, 0.0}, 2e-6);
  close(f32_result.force, {10.0}, 2e-5);

  const structure::Mesh space_mesh{
      pluss::series::make({{0.0, 0.0, 0.0}, {1.0, 2.0, 2.0}})
          .to(pluss::ScalarType::Float64),
      {0},
      {1}};
  const auto space = structure::truss(
      space_mesh, {pluss::series::make({3.0}).to(pluss::ScalarType::Float64),
                   pluss::series::make({12.0}).to(pluss::ScalarType::Float64)});
  const structure::Fix space_fix{{0, 0, 0, 1, 1}, {0, 1, 2, 0, 1}, {}};
  const auto space_result =
      structure::solve(space,
                       pluss::series::make({{0.0, 0.0, 0.0}, {0.0, 0.0, 12.0}})
                           .to(pluss::ScalarType::Float64),
                       space_fix);
  close(space_result.displacement, {0.0, 0.0, 0.0, 0.0, 0.0, 2.25}, 1e-8);
  close(space_result.force, {18.0}, 1e-8);
  close(space_result.reaction, {-6.0, -12.0, -12.0, 6.0, 12.0, 0.0}, 1e-8);

  auto tracked_load = bar_load(10.0);
  tracked_load.tracks_slope_(true);
  structure::solve(truss, tracked_load, bar_fix()).energy.reverse();
  close(tracked_load.sensitivity(), {0.0, 0.0, 0.1, 0.0}, 1e-8);

  auto tracked_area = pluss::series::make({2.0}).to(pluss::ScalarType::Float64);
  tracked_area.tracks_slope_(true);
  const auto tracked_truss = structure::truss(
      bar_mesh(), {tracked_area, pluss::series::make({100.0}).to(
                                     pluss::ScalarType::Float64)});
  structure::solve(tracked_truss, bar_load(10.0), bar_fix()).energy.reverse();
  close(tracked_area.sensitivity(), {-0.25}, 1e-7);

  auto tracked_position = pluss::series::make({{0.0, 0.0}, {2.0, 0.0}})
                              .to(pluss::ScalarType::Float64);
  tracked_position.tracks_slope_(true);
  const auto geometric =
      structure::truss({tracked_position, {0}, {1}}, bar_props());
  structure::solve(geometric, bar_load(10.0), bar_fix()).energy.reverse();
  close(tracked_position.sensitivity(), {-0.25, 0.0, 0.25, 0.0}, 1e-7);

  const auto single = structure::truss(
      {pluss::series::make({{0.0, 0.0}, {1.0, 0.0}})
           .to(pluss::ScalarType::Float64),
       {0},
       {1}},
      {pluss::series::make({1.0}).to(pluss::ScalarType::Float64),
       pluss::series::make({1.0}).to(pluss::ScalarType::Float64)});
  expect_error(
      [&] {
        (void)structure::solve(single, bar_load(1.0), {{0, 0}, {0, 1}, {}});
      },
      "unstiff transverse dof was accepted");

  const auto triangle = structure::truss(
      {pluss::series::make({{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}})
           .to(pluss::ScalarType::Float64),
       {0, 0, 1},
       {1, 2, 2}},
      {pluss::series::ones(
           {3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)),
       pluss::series::ones(
           {3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))});
  expect_error(
      [&] {
        (void)structure::solve(
            triangle,
            pluss::series::make({{0.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}})
                .to(pluss::ScalarType::Float64),
            {{0, 0}, {0, 1}, {}});
      },
      "rotational mechanism was accepted");
  structure::SolveOptions diagnostic_options;
  diagnostic_options.fail_on_nonconvergence = false;
  const auto diagnostic = structure::solve(
      triangle,
      pluss::series::make({{0.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}})
          .to(pluss::ScalarType::Float64),
      {{0, 0}, {0, 1}, {}}, diagnostic_options);
  expect(!diagnostic.converged.item<bool>() &&
             diagnostic.breakdown.item<bool>(),
         "diagnostic solve hid the rotational mechanism");

  expect_error(
      [&] {
        (void)structure::truss({pluss::series::make({{0.0, 0.0}, {0.0, 0.0}})
                                    .to(pluss::ScalarType::Float64),
                                {0},
                                {1}},
                               bar_props());
      },
      "zero-length bar was accepted");
  expect_error(
      [&] {
        (void)structure::truss(
            bar_mesh(),
            {pluss::series::make({-1.0}).to(pluss::ScalarType::Float64),
             pluss::series::make({100.0}).to(pluss::ScalarType::Float64)});
      },
      "negative area was accepted");
  expect_error(
      [&] {
        (void)structure::solve(truss, bar_load(1.0), {{0, 0}, {0, 0}, {}});
      },
      "duplicate constraint was accepted");

  std::cout << "eng structure checks passed\n";
  return 0;
}
