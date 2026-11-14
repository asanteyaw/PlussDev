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

static_assert(!std::is_default_constructible_v<structure::Frame>);
static_assert(std::is_copy_constructible_v<structure::Frame>);

namespace {

void expect(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}

void close(const pluss::Series &value, const std::vector<double> &expected,
           double tolerance = 1e-8) {
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

pluss::Series f64(std::initializer_list<double> values) {
  return pluss::series::make(values).to(pluss::ScalarType::Float64);
}

structure::Mesh one_member() {
  return {pluss::series::make({{0.0, 0.0}, {2.0, 0.0}})
              .to(pluss::ScalarType::Float64),
          {0},
          {1}};
}

structure::Sections section(pluss::Series inertia = f64({3.0}),
                            pluss::Series density = f64({5.0})) {
  return {f64({2.0}), std::move(inertia), f64({100.0}), std::move(density)};
}

structure::Fix fixed_root() { return {{0, 0, 0}, {0, 1, 2}, {}}; }

pluss::Series tip_load(double force) {
  return pluss::series::make({{0.0, 0.0, 0.0}, {0.0, force, 0.0}})
      .to(pluss::ScalarType::Float64);
}

} // namespace

int main() {
  const auto beam = structure::frame(one_member(), section());
  expect(beam.nodes() == 2 && beam.members() == 1 && beam.dofs() == 3,
         "compiled frame metadata is wrong");

  const auto cantilever = structure::solve(beam, tip_load(12.0), fixed_root());
  close(cantilever.displacement,
        {0.0, 0.0, 0.0, 0.0, 0.10666666666666667, 0.08});
  close(cantilever.reaction, {0.0, -12.0, -24.0, 0.0, 0.0, 0.0});
  close(cantilever.end_force, {0.0, -12.0, -24.0, 0.0, 12.0, 0.0});
  close(cantilever.energy, {0.64});
  expect(cantilever.converged.item<bool>() &&
             !cantilever.breakdown.item<bool>(),
         "cantilever solve did not converge");

  const double root2 = std::sqrt(2.0);
  const auto inclined =
      structure::frame({pluss::series::make({{0.0, 0.0}, {root2, root2}})
                            .to(pluss::ScalarType::Float64),
                        {0},
                        {1}},
                       section());
  const auto inclined_result = structure::solve(
      inclined,
      pluss::series::make({{0.0, 0.0, 0.0}, {-12.0 / root2, 12.0 / root2, 0.0}})
          .to(pluss::ScalarType::Float64),
      fixed_root());
  close(inclined_result.displacement,
        {0.0, 0.0, 0.0, -0.10666666666666667 / root2,
         0.10666666666666667 / root2, 0.08});
  close(inclined_result.end_force, {0.0, -12.0, -24.0, 0.0, 12.0, 0.0});

  const auto chain = structure::frame(
      {pluss::series::make({{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}})
           .to(pluss::ScalarType::Float64),
       {0, 1},
       {1, 2}},
      {f64({2.0, 2.0}), f64({3.0, 3.0}), f64({100.0, 100.0}), f64({5.0, 5.0})});
  const auto chain_result = structure::solve(
      chain,
      pluss::series::make({{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 12.0, 0.0}})
          .to(pluss::ScalarType::Float64),
      fixed_root());
  close(chain_result.displacement, {0.0, 0.0, 0.0, 0.0, 0.03333333333333333,
                                    0.06, 0.0, 0.10666666666666667, 0.08});
  close(chain_result.reaction,
        {0.0, -12.0, -24.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});

  const auto batched = structure::frame(
      one_member(),
      section(pluss::series::make(std::vector<double>{3.0, 6.0}, {2, 1}),
              f64({5.0})));
  const auto batched_result =
      structure::solve(batched, tip_load(12.0), fixed_root());
  close(batched_result.displacement,
        {0.0, 0.0, 0.0, 0.0, 0.10666666666666667, 0.08, 0.0, 0.0, 0.0, 0.0,
         0.05333333333333334, 0.04});
  close(batched_result.energy, {0.64, 0.32});

  const auto load_batch = pluss::series::make(
      std::vector<double>{0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 24, 0}, {2, 2, 3});
  const auto load_cases = structure::solve(beam, load_batch, fixed_root());
  close(load_cases.energy, {0.64, 2.56});
  expect(load_cases.displacement.sizes() == std::vector<std::int64_t>{2, 2, 3},
         "load-case axis was not preserved");

  const auto unit_axial =
      pluss::series::make({{1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}})
          .to(pluss::ScalarType::Float64);
  close(structure::mass(beam, unit_axial), {10.0, 0.0, 0.0, 10.0, 0.0, 0.0});
  const auto general_acceleration =
      pluss::series::make({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}})
          .to(pluss::ScalarType::Float64);
  close(structure::mass(beam, general_acceleration),
        {20.0, 558.0 / 21.0, 194.0 / 21.0, 30.0, 702.0 / 21.0, -212.0 / 21.0});
  const auto probe = pluss::series::make({{-2.0, 1.0, 0.5}, {3.0, -4.0, 2.0}})
                         .to(pluss::ScalarType::Float64);
  const double mass_xy = general_acceleration.mul(structure::mass(beam, probe))
                             .sum()
                             .item<double>();
  const double mass_yx = probe.mul(structure::mass(beam, general_acceleration))
                             .sum()
                             .item<double>();
  expect(std::abs(mass_xy - mass_yx) < 1e-10,
         "consistent mass operator is not symmetric");

  const auto beam32 = structure::frame(
      {one_member().position.to(pluss::ScalarType::Float32), {0}, {1}},
      {section().area.to(pluss::ScalarType::Float32),
       section().inertia.to(pluss::ScalarType::Float32),
       section().modulus.to(pluss::ScalarType::Float32),
       section().density.to(pluss::ScalarType::Float32)});
  const auto result32 = structure::solve(
      beam32, tip_load(12.0).to(pluss::ScalarType::Float32), fixed_root());
  close(result32.displacement, {0.0, 0.0, 0.0, 0.0, 0.10666666666666667, 0.08},
        2e-5);
  expect(result32.displacement.scalar_type() == pluss::ScalarType::Float32,
         "frame solve widened Float32");
  const auto mass32 =
      structure::mass(beam32, unit_axial.to(pluss::ScalarType::Float32));
  close(mass32, {10.0, 0.0, 0.0, 10.0, 0.0, 0.0}, 2e-5);
  expect(mass32.scalar_type() == pluss::ScalarType::Float32,
         "frame mass action widened Float32");

  auto tracked_load = tip_load(12.0);
  tracked_load.tracks_slope_(true);
  structure::solve(beam, tracked_load, fixed_root()).energy.reverse();
  close(tracked_load.sensitivity(),
        {0.0, 0.0, 0.0, 0.0, 0.10666666666666667, 0.08}, 2e-7);

  auto tracked_inertia = f64({3.0});
  tracked_inertia.tracks_slope_(true);
  const auto tracked_beam =
      structure::frame(one_member(), section(tracked_inertia));
  structure::solve(tracked_beam, tip_load(12.0), fixed_root()).energy.reverse();
  close(tracked_inertia.sensitivity(), {-0.21333333333333335}, 2e-6);

  auto tracked_density = f64({5.0});
  tracked_density.tracks_slope_(true);
  const auto mass_beam =
      structure::frame(one_member(), section(f64({3.0}), tracked_density));
  structure::mass(mass_beam, unit_axial).sum().reverse();
  close(tracked_density.sensitivity(), {4.0}, 2e-7);

  expect_error(
      [&] {
        (void)structure::frame(
            one_member(), {f64({2.0}), f64({-3.0}), f64({100.0}), f64({5.0})});
      },
      "negative inertia was accepted");
  expect_error(
      [&] {
        const auto no_mass = structure::frame(
            one_member(), {f64({2.0}), f64({3.0}), f64({100.0}), {}});
        (void)structure::mass(no_mass, unit_axial);
      },
      "missing density was accepted by mass");
  expect_error(
      [&] {
        (void)structure::frame(
            {pluss::series::make({{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}})
                 .to(pluss::ScalarType::Float64),
             {0},
             {1}},
            section());
      },
      "three-dimensional frame was accepted as planar");
  expect_error(
      [&] {
        (void)structure::solve(beam, tip_load(12.0), {{0, 0}, {0, 1}, {}});
      },
      "underconstrained frame was accepted");

  // A uniformly loaded simply supported beam (two members): the node deflection, support reactions and the end action at mid-span
  // must be exact, and the member's own load must be carried in end_force.
  {
    constexpr double span = 4.0, w = 1000.0, modulus = 200e9, inertia = 1e-7;
    const auto positions = pluss::series::make({0.0, 0.0, 2.0, 0.0, 4.0, 0.0})
                               .reshape({3, 2})
                               .to(pluss::ScalarType::Float64);
    const auto fill = [](double value) {
      return pluss::series::make(std::vector<double>(2, value))
          .to(pluss::ScalarType::Float64);
    };
    const auto simple = structure::frame({positions, {0, 1}, {1, 2}},
                                         {fill(1e-3), fill(inertia), fill(modulus)});
    const structure::MemberLoad uniform{fill(-w), {}};
    const auto none = pluss::series::zeros(
        {3, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
    const auto r = structure::solve(simple, none, uniform, {{0, 0, 2}, {0, 1, 1}, {}});
    const double mid = -5.0 * w * std::pow(span, 4) / (384.0 * modulus * inertia);
    expect(std::abs(r.displacement.select(-2, 1).select(-1, 1).item<double>() - mid) < 1e-12,
           "uniform member load deflection is wrong");
    expect(std::abs(r.reaction.select(-2, 0).select(-1, 1).item<double>() - w * span / 2) < 1e-6,
           "uniform member load reaction is wrong");
    // a batch of three intensities joins the leading axes: the middle one reproduces the single solve
    const structure::MemberLoad batched{
        pluss::series::make(std::vector<double>{-0.5 * w, -0.5 * w, -w, -w, -2.0 * w, -2.0 * w})
            .reshape({3, 2})
            .to(pluss::ScalarType::Float64),
        {}};
    const auto many = structure::solve(simple, none, batched, {{0, 0, 2}, {0, 1, 1}, {}});
    expect(many.displacement.size(0) == 3 &&
               std::abs(many.displacement.select(0, 1).select(-2, 1).select(-1, 1).item<double>() - mid) < 1e-12,
           "a batch of member loads did not broadcast");
    // Mj of the first member is the sagging moment at mid-span, w L^2 / 8
    expect(std::abs(r.end_force.select(-2, 0).select(-1, 5).item<double>() - w * span * span / 8) < 1e-6,
           "uniform member load end action is wrong");
  }

  std::cout << "eng frame checks passed\n";
  return 0;
}
