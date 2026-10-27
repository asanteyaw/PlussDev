#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace quantum = pluss::subsp::natsci::physics::quantum;

namespace {

using Complex = std::complex<double>;

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool close(double left, double right, double tolerance = 1e-11) {
  return std::abs(left - right) <=
         tolerance * std::max({1.0, std::abs(left), std::abs(right)});
}

pluss::Series complex_series(const std::vector<Complex> &values,
                             const std::vector<std::int64_t> &shape) {
  return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128);
}

void test_state_foundation() {
  const auto raw = quantum::ket(pluss::make_series({3.0, 4.0}));
  expect(close(quantum::norm(raw).item<double>(), 5.0),
         "ket norm is incorrect");
  const auto unit = quantum::normalize(raw);
  expect(unit.value().allclose(pluss::make_series({0.6, 0.8}), 1e-12, 1e-12) &&
             close(quantum::norm(unit).item<double>(), 1.0),
         "ket normalization is incorrect");

  const auto batch = quantum::normalize(
      quantum::ket(pluss::make_series({{3.0, 4.0}, {0.0, 2.0}})));
  expect(batch.batch_shape() == std::vector<std::int64_t>{2} &&
             quantum::norm(batch).allclose(pluss::make_series({1.0, 1.0}),
                                           1e-12, 1e-12),
         "batched ket normalization changed shape or value");

  const auto phase = quantum::ket(
      complex_series({{1.0, 0.0}, {0.0, 1.0}}, {2}).div(std::sqrt(2.0)));
  expect(std::abs(quantum::inner(phase, phase).item<Complex>() -
                  Complex{1, 0}) < 1e-12,
         "complex inner product did not conjugate the left state");
  expect(quantum::prob(phase).allclose(pluss::make_series({0.5, 0.5}), 1e-12,
                                       1e-12),
         "Born probabilities are incorrect");
}

void test_operator_and_density() {
  const auto phase = quantum::ket(
      complex_series({{1.0, 0.0}, {0.0, 1.0}}, {2}).div(std::sqrt(2.0)));
  const auto rho = quantum::density(phase);
  const auto expected_rho =
      complex_series({{0.5, 0.0}, {0.0, -0.5}, {0.0, 0.5}, {0.5, 0.0}}, {2, 2});
  expect(rho.value().allclose(expected_rho, 1e-12, 1e-12),
         "pure-state density matrix is incorrect");
  expect(std::abs(quantum::trace(rho).item<Complex>() - Complex{1, 0}) <
                 1e-12 &&
             std::abs(quantum::purity(rho).item<Complex>() - Complex{1, 0}) <
                 1e-12,
         "density trace or purity is incorrect");

  const auto sigma_y = quantum::op(complex_series(
      {{0.0, 0.0}, {0.0, -1.0}, {0.0, 1.0}, {0.0, 0.0}}, {2, 2}));
  expect(quantum::is_herm(sigma_y).item<bool>(),
         "Pauli Y was not recognized as Hermitian");
  expect(std::abs(quantum::expect(sigma_y, phase).item<Complex>() -
                  Complex{1, 0}) < 1e-12,
         "pure-state expectation is incorrect");
  expect(std::abs(quantum::expect(sigma_y, rho).item<Complex>() -
                  Complex{1, 0}) < 1e-12,
         "density-matrix expectation is incorrect");

  const auto nonherm = quantum::op(
      complex_series({{0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  expect(!quantum::is_herm(nonherm).item<bool>() &&
             quantum::dag(nonherm).value().allclose(nonherm.value().adjoint(),
                                                    1e-12, 1e-12),
         "adjoint or Hermiticity check is incorrect");
}

void test_tensor_and_partial_trace() {
  const auto zero = quantum::ket(pluss::make_series({1.0, 0.0}));
  const auto one = quantum::ket(pluss::make_series({0.0, 1.0}));
  const auto zero_one = quantum::tensor(zero, one);
  expect(zero_one.spaces() == std::vector<std::int64_t>({2, 2}) &&
             zero_one.value().allclose(pluss::make_series({0.0, 1.0, 0.0, 0.0}),
                                       1e-12, 1e-12),
         "ket tensor product is incorrect");

  const auto bell =
      quantum::ket(pluss::make_series(
                       {1.0 / std::sqrt(2.0), 0.0, 0.0, 1.0 / std::sqrt(2.0)}),
                   {2, 2});
  const auto bell_density = quantum::density(bell);
  const auto reduced_left = quantum::partial_trace(bell_density, {0});
  const auto reduced_right = quantum::partial_trace(bell_density, {1});
  const auto mixed = pluss::make_series({{0.5, 0.0}, {0.0, 0.5}});
  expect(reduced_left.value().allclose(mixed, 1e-12, 1e-12) &&
             reduced_right.value().allclose(mixed, 1e-12, 1e-12) &&
             close(quantum::purity(reduced_left).item<double>(), 0.5),
         "Bell-state partial trace is incorrect");

  const auto bell_batch =
      quantum::ket(bell.value().reshape({1, 1, 4}).expand({2, 3, 4}), {2, 2});
  const auto reduced_batch =
      quantum::partial_trace(quantum::density(bell_batch), {0});
  expect(
      reduced_batch.value().sizes() ==
              std::vector<std::int64_t>({2, 3, 2, 2}) &&
          reduced_batch.value().allclose(
              mixed.reshape({1, 1, 2, 2}).expand({2, 3, 2, 2}), 1e-12, 1e-12),
      "batched Bell-state partial trace changed shape or values");

  const auto ghz =
      quantum::ket(pluss::make_series({1.0 / std::sqrt(2.0), 0.0, 0.0, 0.0, 0.0,
                                       0.0, 0.0, 1.0 / std::sqrt(2.0)}),
                   {2, 2, 2});
  const auto ghz_edges = quantum::partial_trace(quantum::density(ghz), {0, 2});
  const auto expected_edges = pluss::make_series({{0.5, 0.0, 0.0, 0.0},
                                                  {0.0, 0.0, 0.0, 0.0},
                                                  {0.0, 0.0, 0.0, 0.0},
                                                  {0.0, 0.0, 0.0, 0.5}});
  expect(ghz_edges.spaces() == std::vector<std::int64_t>({2, 2}) &&
             ghz_edges.value().allclose(expected_edges, 1e-12, 1e-12),
         "partial trace over a middle subsystem is incorrect");

  const auto scalar = quantum::partial_trace(bell_density, {});
  expect(scalar.spaces() == std::vector<std::int64_t>{1} &&
             scalar.value().sizes() == std::vector<std::int64_t>({1, 1}) &&
             close(scalar.value().host<double>().front(), 1.0),
         "tracing every subsystem did not produce the trivial density space");

  const auto z = quantum::op(pluss::make_series({{1.0, 0.0}, {0.0, -1.0}}));
  const auto x = quantum::op(pluss::make_series({{0.0, 1.0}, {1.0, 0.0}}));
  const auto zx = quantum::tensor(z, x);
  expect(zx.spaces() == std::vector<std::int64_t>({2, 2}) &&
             zx.value().sizes() == std::vector<std::int64_t>({4, 4}) &&
             quantum::is_herm(zx).item<bool>(),
         "operator tensor product is incorrect");
}

void test_batches_views_and_reverse() {
  const auto states =
      quantum::ket(pluss::make_series({{1.0, 0.0}, {0.0, 1.0}}));
  const auto z = quantum::op(pluss::make_series({{1.0, 0.0}, {0.0, -1.0}}));
  expect(quantum::expect(z, states).allclose(pluss::make_series({1.0, -1.0}),
                                             1e-12, 1e-12),
         "observable did not broadcast across the state batch");

  const pluss::Series storage =
      pluss::make_series({{1.0, 0.0, 0.0}, {0.0, 1.0, 1.0}});
  const auto strided = quantum::ket(storage.transpose(0, 1));
  expect(strided.batch_shape() == std::vector<std::int64_t>{3} &&
             quantum::prob(strided).allclose(
                 pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {0.0, 1.0}}),
                 1e-12, 1e-12),
         "strided state storage changed values");

  auto theta = pluss::series::make(0.37);
  theta.tracks_slope_(true);
  const pluss::Series amplitudes =
      theta.cos().unsqueeze(0).unite({theta.sin().unsqueeze(0)}, 0);
  quantum::expect(z, quantum::ket(amplitudes)).reverse();
  const double expected = -2.0 * std::sin(2.0 * 0.37);
  expect(theta.has_sensitivity() &&
             close(theta.sensitivity().item<double>(), expected, 2e-10),
         "quantum expectation severed reverse sensitivity");
}

void test_failures() {
  bool bad_space = false;
  try {
    (void)quantum::ket(pluss::make_series({1.0, 0.0}), {2, 2});
  } catch (const std::invalid_argument &) {
    bad_space = true;
  }
  expect(bad_space, "ket accepted incompatible subsystem dimensions");

  bool zero_state = false;
  try {
    (void)quantum::normalize(quantum::ket(pluss::make_series({0.0, 0.0})));
  } catch (const std::domain_error &) {
    zero_state = true;
  }
  expect(zero_state, "normalize accepted the zero vector");

  bool nonsquare = false;
  try {
    (void)quantum::op(pluss::make_series({{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}}));
  } catch (const std::invalid_argument &) {
    nonsquare = true;
  }
  expect(nonsquare, "op accepted a non-square matrix");

  bool duplicate = false;
  try {
    const auto rho = quantum::density(
        quantum::ket(pluss::make_series({1.0, 0.0, 0.0, 0.0}), {2, 2}));
    (void)quantum::partial_trace(rho, {0, 0});
  } catch (const std::invalid_argument &) {
    duplicate = true;
  }
  expect(duplicate, "partial_trace accepted duplicate subsystem indices");
}

} // namespace

int main() {
  try {
    test_state_foundation();
    test_operator_and_density();
    test_tensor_and_partial_trace();
    test_batches_views_and_reverse();
    test_failures();
    std::cout << "NatSci shared quantum foundation checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
