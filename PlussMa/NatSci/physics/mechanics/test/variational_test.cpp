#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace mechanics = pluss::subsp::natsci::physics::mechanics;

namespace {

using Coordinate = common::Length;
using Velocity = mechanics::CoordinateRate<Coordinate>;
using Momentum = mechanics::CanonicalMomentum<Coordinate>;

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool close(double actual, double expected, double relative = 1e-8) {
  const double scale = std::max({1.0, std::abs(actual), std::abs(expected)});
  return std::abs(actual - expected) <= relative * scale;
}

void expect_shape(const pluss::Series &value,
                  std::initializer_list<std::int64_t> expected,
                  const std::string &message) {
  expect(value.dim() == static_cast<std::int64_t>(expected.size()), message);
  std::size_t axis = 0;
  for (const auto extent : expected)
    expect(value.size(static_cast<std::int64_t>(axis++)) == extent, message);
}

auto harmonic_hamiltonian = [](const common::SeriesQuantity<Coordinate> &q,
                               const common::SeriesQuantity<Momentum> &p,
                               const common::SeriesQuantity<common::Time> &) {
  return common::SeriesQuantity<common::Energy>::from_si(
      q.si_value().square().add(p.si_value().square()).mul(0.5).sum(-1, false));
};

auto harmonic_potential = [](const common::SeriesQuantity<Coordinate> &q,
                             const common::SeriesQuantity<common::Time> &) {
  return common::SeriesQuantity<common::Energy>::from_si(
      q.si_value().square().mul(0.5).sum(-1, false));
};

auto harmonic_kinetic = [](const common::SeriesQuantity<Momentum> &p,
                           const common::SeriesQuantity<common::Time> &) {
  return common::SeriesQuantity<common::Energy>::from_si(
      p.si_value().square().mul(0.5).sum(-1, false));
};

auto harmonic_lagrangian = [](const common::SeriesQuantity<Coordinate> &q,
                              const common::SeriesQuantity<Velocity> &v,
                              const common::SeriesQuantity<common::Time> &) {
  return common::SeriesQuantity<common::Energy>::from_si(
      v.si_value().square().sub(q.si_value().square()).mul(0.5).sum(-1, false));
};

void test_states_and_hamiltonian_flow() {
  const auto state = mechanics::make_phase(
      common::SeriesQuantity<Coordinate>::from_si(
          pluss::series::make({{1.0, 2.0}, {3.0, 4.0}})),
      common::SeriesQuantity<Momentum>::from_si(
          pluss::series::make({{0.5, -1.0}, {2.0, -3.0}})));
  expect(state.dof() == 2 &&
             state.batch_shape() == std::vector<std::int64_t>{2},
         "phase state lost its semantic axes");

  const auto evaluated = mechanics::hamiltonian(harmonic_hamiltonian, state);
  expect_shape(evaluated.value.si_value(), {2},
               "batched Hamiltonian changed the batch shape");
  expect(evaluated.value.si_value().allclose(pluss::series::make({3.125, 19.0}),
                                             1e-12, 1e-12),
         "Hamiltonian value is incorrect");
  expect(evaluated.flow.coordinate.si_value().allclose(
             state.momentum().si_value(), 1e-12, 1e-12),
         "Hamilton equation dq/dt = dH/dp is incorrect");
  expect(evaluated.flow.momentum.si_value().allclose(
             state.coordinate().si_value().neg(), 1e-12, 1e-12),
         "Hamilton equation dp/dt = -dH/dq is incorrect");
}

void test_symplectic_step_and_reverse_time() {
  auto q = pluss::series::make({1.0});
  q.tracks_slope_(true);
  const auto initial = mechanics::make_phase(
      common::SeriesQuantity<Coordinate>::from_si(q),
      common::SeriesQuantity<Momentum>::from_si(pluss::series::make({0.0})));
  const auto step = common::ScalarQuantity<common::Time>::from_si(0.1);
  auto potential = harmonic_potential;
  auto kinetic = harmonic_kinetic;
  const pluss::Series t = mechanics::hamiltonian_detail::batch_time(
      q, common::ScalarQuantity<common::Time>::from_si(0.0));
  const pluss::Series force = mechanics::hamiltonian_detail::energy_gradient<
      decltype(potential), Coordinate>(potential, q, t, {}, "test potential");
  expect(force.allclose(q, 1e-12, 1e-12),
         "separable potential gradient is incorrect");
  const pluss::Series half_p =
      initial.momentum().si_value().sub(force.mul(0.05));
  const pluss::Series speed = mechanics::hamiltonian_detail::energy_gradient<
      decltype(kinetic), Momentum>(kinetic, half_p, t, {}, "test kinetic");
  expect(speed.allclose(half_p, 1e-12, 1e-12),
         "separable kinetic gradient is incorrect");
  const auto next = mechanics::symplectic_step(
      initial, step, harmonic_potential, harmonic_kinetic);
  const double next_q = next.coordinate().si_value().host<double>().front();
  const double next_p = next.momentum().si_value().host<double>().front();
  expect(close(next_q, 0.995),
         "symplectic position update is incorrect: " + std::to_string(next_q));
  expect(close(next_p, -0.09975),
         "symplectic momentum update is incorrect: " + std::to_string(next_p));

  next.coordinate().si_value().sum().reverse();
  expect(q.has_sensitivity(),
         "symplectic step severed the outer reverse graph");
  expect(close(q.sensitivity().host<double>().front(), 0.995, 1e-7),
         "symplectic step returned the wrong coordinate sensitivity");

  const auto restored = mechanics::symplectic_step(
      next, common::ScalarQuantity<common::Time>::from_si(-0.1),
      harmonic_potential, harmonic_kinetic,
      common::ScalarQuantity<common::Time>::from_si(0.1));
  expect(restored.coordinate().si_value().allclose(
             initial.coordinate().si_value(), 1e-10, 1e-10) &&
             restored.momentum().si_value().allclose(
                 initial.momentum().si_value(), 1e-10, 1e-10),
         "negative symplectic step did not reverse the trajectory");

  const auto unchanged = mechanics::symplectic_step(
      initial, common::ScalarQuantity<common::Time>::from_si(0.0),
      harmonic_potential, harmonic_kinetic);
  expect(
      unchanged.coordinate().si_value().equal(
          initial.coordinate().si_value()) &&
          unchanged.momentum().si_value().equal(initial.momentum().si_value()),
      "zero symplectic step is not the identity");
}

void test_lagrange_field_and_batches() {
  const auto state = mechanics::make_tangent(
      common::SeriesQuantity<Coordinate>::from_si(
          pluss::series::make({{1.0, 2.0}, {3.0, 4.0}})),
      common::SeriesQuantity<Velocity>::from_si(
          pluss::series::make({{0.5, -1.0}, {2.0, -3.0}})));
  const auto evaluated = mechanics::lagrange(harmonic_lagrangian, state);
  expect_shape(evaluated.value.si_value(), {2},
               "batched Lagrangian changed the batch shape");
  expect(evaluated.momentum.si_value().allclose(state.velocity().si_value(),
                                                1e-7, 1e-7),
         "generalized momentum is incorrect");
  expect_shape(evaluated.mass.si_value(), {2, 2, 2},
               "batched velocity Hessian has the wrong shape");
  expect(evaluated.mass.si_value().allclose(
             pluss::series::eye(2, pluss::ScalarType::Float64)
                 .unsqueeze(0)
                 .expand({2, 2, 2}),
             1e-7, 1e-7),
         "batched generalized mass matrix is incorrect");
  expect(evaluated.flow.velocity.si_value().allclose(
             state.coordinate().si_value().neg(), 1e-6, 1e-6),
         "Euler-Lagrange acceleration is incorrect");
}

void test_explicit_time_mixed_partial() {
  const auto state =
      mechanics::make_tangent(common::SeriesQuantity<Coordinate>::from_si(
                                  pluss::series::make({1.5, -2.0})),
                              common::SeriesQuantity<Velocity>::from_si(
                                  pluss::series::make({0.25, 0.75})));
  const auto time_lagrangian = [](const common::SeriesQuantity<Coordinate> &q,
                                  const common::SeriesQuantity<Velocity> &v,
                                  const common::SeriesQuantity<common::Time>
                                      &t) {
    const pluss::Series value =
        v.si_value()
            .square()
            .mul(0.5)
            .add(t.si_value().unsqueeze(-1).mul(q.si_value()).mul(v.si_value()))
            .sub(q.si_value().square().mul(0.5))
            .sum(-1, false);
    return common::SeriesQuantity<common::Energy>::from_si(value);
  };
  const auto evaluated =
      mechanics::lagrange(time_lagrangian, state,
                          common::ScalarQuantity<common::Time>::from_si(0.3));
  expect(evaluated.flow.velocity.si_value().allclose(
             state.coordinate().si_value().mul(-2.0), 1e-6, 1e-6),
         "Euler-Lagrange flow omitted the explicit-time mixed partial");
}

void test_drift_and_failures() {
  const auto reference = common::SeriesQuantity<common::Energy>::from_si(
      pluss::series::make({10.0, 0.0}));
  const auto current = common::SeriesQuantity<common::Energy>::from_si(
      pluss::series::make({11.0, 0.0}));
  const auto report =
      common::drift(reference, current,
                    common::ScalarQuantity<common::Energy>::from_si(10.0));
  expect(report.absolute.si_value().equal(pluss::series::make({1.0, 0.0})),
         "absolute invariant drift is incorrect");
  expect(report.scaled.si_value().allclose(pluss::series::make({0.1, 0.0}),
                                           1e-12, 1e-12),
         "scaled invariant drift is incorrect");

  bool wrong_shape = false;
  try {
    (void)mechanics::make_phase(
        common::SeriesQuantity<Coordinate>::from_si(
            pluss::series::make({1.0, 2.0})),
        common::SeriesQuantity<Momentum>::from_si(pluss::series::make({1.0})));
  } catch (const std::invalid_argument &) {
    wrong_shape = true;
  }
  expect(wrong_shape, "phase state accepted mismatched canonical arrays");

  bool wrong_energy_shape = false;
  try {
    const auto state =
        mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(
                                  pluss::series::make({1.0, 2.0})),
                              common::SeriesQuantity<Momentum>::from_si(
                                  pluss::series::make({0.0, 0.0})));
    const auto invalid = [](const common::SeriesQuantity<Coordinate> &q,
                            const common::SeriesQuantity<Momentum> &,
                            const common::SeriesQuantity<common::Time> &) {
      return common::SeriesQuantity<common::Energy>::from_si(
          q.si_value().square());
    };
    (void)mechanics::hamiltonian(invalid, state);
  } catch (const std::invalid_argument &) {
    wrong_energy_shape = true;
  }
  expect(wrong_energy_shape,
         "Hamiltonian accepted an output that was neither scalar nor batched");

  bool nonfinite_step = false;
  try {
    const auto state = mechanics::make_phase(
        common::SeriesQuantity<Coordinate>::from_si(pluss::series::make({1.0})),
        common::SeriesQuantity<Momentum>::from_si(pluss::series::make({0.0})));
    (void)mechanics::symplectic_step(
        state,
        common::ScalarQuantity<common::Time>::from_si(
            std::numeric_limits<double>::infinity()),
        harmonic_potential, harmonic_kinetic);
  } catch (const std::invalid_argument &) {
    nonfinite_step = true;
  }
  expect(nonfinite_step, "symplectic step accepted infinite time");
}

} // namespace

int main() {
  try {
    test_states_and_hamiltonian_flow();
    test_symplectic_step_and_reverse_time();
    test_lagrange_field_and_batches();
    test_explicit_time_mixed_partial();
    test_drift_and_failures();
    std::cout << "NatSci variational mechanics checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
