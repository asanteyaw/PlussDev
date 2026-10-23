#include "pluss/ma/natsci/physics.h"
#include "pluss/subsp/linearalg/linearalg.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <iostream>
#include <numbers>
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

bool close(double left, double right, double tolerance = 1e-9) {
  return std::abs(left - right) <=
         tolerance * std::max({1.0, std::abs(left), std::abs(right)});
}

pluss::Series cx(const std::vector<Complex> &values,
                 const std::vector<std::int64_t> &shape) {
  return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128);
}

quantum::Ket zero() { return quantum::ket(cx({{1.0, 0.0}, {0.0, 0.0}}, {2})); }

quantum::Ket one() { return quantum::ket(cx({{0.0, 0.0}, {1.0, 0.0}}, {2})); }

quantum::Op pauli_x() {
  return quantum::op(
      cx({{0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}}, {2, 2}));
}

void test_series_complex_eigh() {
  const auto matrices = cx({{0.0, 0.0},
                            {0.0, -1.0},
                            {0.0, 1.0},
                            {0.0, 0.0},
                            {0.0, 0.0},
                            {0.0, -2.0},
                            {0.0, 2.0},
                            {0.0, 0.0}},
                           {2, 2, 2});
  const auto decomposition = matrices.linearalg.eigh();
  expect(decomposition.values.sizes() == std::vector<std::int64_t>({2, 2}) &&
             decomposition.vectors.sizes() ==
                 std::vector<std::int64_t>({2, 2, 2}),
         "batched complex eigh returned the wrong shapes");
  expect(decomposition.values.allclose(
             pluss::make_series({{-1.0, 1.0}, {-2.0, 2.0}}), 1e-12, 1e-12),
         "batched complex eigh returned the wrong eigenvalues");
  const auto rebuilt =
      decomposition.vectors.mul(decomposition.values.unsqueeze(-2))
          .matmul(decomposition.vectors.adjoint());
  expect(rebuilt.allclose(matrices, 1e-11, 1e-11),
         "complex eigh did not reconstruct its Hermitian input");
}

double projector_loss(double off_diagonal) {
  const auto matrix =
      pluss::make_series({{2.0, off_diagonal}, {off_diagonal, 4.0}});
  const auto vectors = matrix.linearalg.eigh().vectors;
  const auto ground = vectors.select(-1, 0);
  const auto projector = ground.unsqueeze(-1).matmul(ground.unsqueeze(-2));
  return projector.mul(pluss::make_series({{0.3, -0.2}, {-0.2, 0.7}}))
      .sum()
      .item<double>();
}

void test_series_eigh_reverse() {
  auto diagonal = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});
  diagonal.tracks_slope_(true);
  diagonal.linearalg.eigvalsh()
      .mul(pluss::make_series({2.0, 5.0}))
      .sum()
      .reverse();
  expect(diagonal.sensitivity().allclose(
             pluss::make_series({{2.0, 0.0}, {0.0, 5.0}}), 1e-12, 1e-12),
         "eigvalsh reverse is incorrect at a repeated spectrum");

  auto matrix = pluss::make_series({{2.0, 0.6}, {0.6, 4.0}});
  matrix.tracks_slope_(true);
  const auto vectors = matrix.linearalg.eigh().vectors;
  const auto ground = vectors.select(-1, 0);
  ground.unsqueeze(-1)
      .matmul(ground.unsqueeze(-2))
      .mul(pluss::make_series({{0.3, -0.2}, {-0.2, 0.7}}))
      .sum()
      .reverse();
  constexpr double epsilon = 1e-6;
  const double numeric =
      (projector_loss(0.6 + epsilon) - projector_loss(0.6 - epsilon)) /
      (2.0 * epsilon);
  const auto gradient = matrix.sensitivity();
  const double analytic = gradient.select(0, 0).select(0, 1).item<double>() +
                          gradient.select(0, 1).select(0, 0).item<double>();
  expect(close(analytic, numeric, 2e-6),
         "eigh eigenvector reverse disagrees with finite differences");

  auto repeated = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});
  repeated.tracks_slope_(true);
  bool rejected = false;
  try {
    repeated.linearalg.eigh().vectors.select(0, 0).select(0, 1).reverse();
  } catch (const std::exception &) {
    rejected = true;
  }
  expect(rejected,
         "eigh accepted an eigenvector loss on a degenerate eigenspace");
}

void test_spectrum_entropy_and_fidelity() {
  const auto hamiltonian =
      quantum::op(pluss::make_series({{-2.0, 0.0}, {0.0, 3.0}}));
  const auto spectral = quantum::spectrum(hamiltonian);
  expect(
      spectral.values.allclose(pluss::make_series({-2.0, 3.0}), 1e-12, 1e-12) &&
          quantum::fidelity(quantum::ground(hamiltonian), zero())
                  .item<double>() > 1.0 - 1e-12,
      "spectrum or ground state is incorrect");

  const auto pure = quantum::density(zero());
  const auto mixed = quantum::op(pluss::make_series({{0.5, 0.0}, {0.0, 0.5}}));
  expect(close(quantum::entropy(pure).item<double>(), 0.0, 1e-12) &&
             close(quantum::entropy(mixed).item<double>(), 1.0, 1e-12),
         "von Neumann entropy is incorrect");
  expect(
      close(quantum::fidelity(zero(), one()).item<double>(), 0.0, 1e-12) &&
          close(quantum::fidelity(zero(), mixed).item<double>(), 0.5, 1e-12) &&
          close(quantum::root_fidelity(zero(), mixed).item<double>(),
                std::sqrt(0.5), 1e-12) &&
          close(quantum::fidelity(mixed, mixed).item<double>(), 1.0, 1e-10),
      "pure or mixed-state fidelity convention is incorrect");
}

void test_unitary_and_schrodinger() {
  const auto hamiltonian = quantum::op(pauli_x().value().mul(0.5));
  const auto propagator = quantum::unitary(hamiltonian, std::numbers::pi);
  const auto identity = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}})
                            .to(pluss::ScalarType::Complex128);
  expect(propagator.value()
             .adjoint()
             .matmul(propagator.value())
             .allclose(identity, 1e-11, 1e-11),
         "Hamiltonian exponential is not unitary");

  const auto times = pluss::make_series(
      {5.0, 5.0 + std::numbers::pi * 0.5, 5.0 + std::numbers::pi});
  const auto states = quantum::schrod(hamiltonian, zero(), times);
  const auto probabilities = quantum::prob(states);
  expect(probabilities.sizes() == std::vector<std::int64_t>({3, 2}) &&
             probabilities.allclose(
                 pluss::make_series({{1.0, 0.0}, {0.5, 0.5}, {0.0, 1.0}}),
                 1e-10, 1e-10),
         "Schrodinger evolution ignored its time origin or phase dynamics");

  const auto real_hamiltonian =
      quantum::op(pluss::make_series({{0.0F, 0.5F}, {0.5F, 0.0F}}));
  const auto promoted = quantum::schrod(real_hamiltonian, zero(),
                                        pluss::make_series({0.0, 0.25}));
  expect(promoted.value().scalar_type() == pluss::ScalarType::Complex128,
         "Schrodinger evolution did not promote a real Hamiltonian to the "
         "complex state's precision");
}

void test_kraus_channel() {
  constexpr double probability = 0.25;
  const auto k0 = quantum::op(cx(
      {{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {std::sqrt(1.0 - probability), 0.0}},
      {2, 2}));
  const auto k1 = quantum::op(
      cx({{0.0, 0.0}, {std::sqrt(probability), 0.0}, {0.0, 0.0}, {0.0, 0.0}},
         {2, 2}));
  const auto damping = quantum::channel({k0, k1});
  const auto output = quantum::apply(damping, one());
  expect(output.value().allclose(cx({{probability, 0.0},
                                     {0.0, 0.0},
                                     {0.0, 0.0},
                                     {1.0 - probability, 0.0}},
                                    {2, 2}),
                                 1e-12, 1e-12) &&
             close(quantum::trace(output).real().item<double>(), 1.0, 1e-12),
         "amplitude-damping channel is incorrect or not trace preserving");

  bool rejected = false;
  try {
    (void)quantum::channel({quantum::op(k0.value().mul(0.5))});
  } catch (const std::domain_error &) {
    rejected = true;
  }
  expect(rejected, "channel accepted an incomplete Kraus family");
}

void test_lindblad_amplitude_damping() {
  constexpr double rate = 0.7;
  const auto zero_h = quantum::op(pluss::series::zeros({2, 2}));
  const auto collapse = quantum::op(
      cx({{0.0, 0.0}, {std::sqrt(rate), 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto times = pluss::make_series({0.0, 0.5, 1.0});
  quantum::SolveOptions options;
  options.steps_per_span = 64;
  const auto path = quantum::lindblad(zero_h, quantum::density(one()), times,
                                      {collapse}, options);
  expect(path.value().sizes() == std::vector<std::int64_t>({3, 2, 2}),
         "Lindblad solver returned the wrong path shape");
  for (std::int64_t index = 0; index < 3; ++index) {
    const double time = times.select(0, index).item<double>();
    const auto rho = path.value().select(0, index);
    const double excited = rho.select(0, 1).select(0, 1).real().item<double>();
    const double ground = rho.select(0, 0).select(0, 0).real().item<double>();
    expect(close(excited, std::exp(-rate * time), 2e-9) &&
               close(ground, 1.0 - std::exp(-rate * time), 2e-9) &&
               close(quantum::trace(quantum::op(rho)).real().item<double>(),
                     1.0, 2e-10),
           "Lindblad amplitude damping disagrees with the analytic solution");
  }
}

void test_time_dependent_and_sparse_dynamics() {
  const double final_time = std::sqrt(2.0 * std::numbers::pi);
  const auto times = pluss::make_series({0.0, final_time * 0.5, final_time});
  quantum::SolveOptions adaptive;
  adaptive.method = quantum::SolveMethod::RK45;
  adaptive.relative_tolerance = 1e-10;
  adaptive.absolute_tolerance = 1e-12;
  adaptive.initial_step = final_time;
  const auto driven = quantum::schrod(
      [](double time) {
        return quantum::op(pauli_x().value().mul(0.5 * time));
      },
      zero(), times, adaptive);
  const auto driven_prob = quantum::prob(driven);
  expect(driven_prob.select(0, 2).allclose(pluss::make_series({0.0, 1.0}), 2e-9,
                                           2e-9),
         "time-dependent Schrodinger evolution used the wrong operator time");

  const auto sparse_h =
      quantum::sparse_op(2, pluss::make_series(std::vector<std::int64_t>{0, 1}),
                         pluss::make_series(std::vector<std::int64_t>{1, 0}),
                         pluss::make_series({0.5, 0.5}), {}, true);
  quantum::SolveOptions krylov_options = adaptive;
  krylov_options.method = quantum::SolveMethod::Krylov;
  krylov_options.linear.max_iterations = 8;
  const auto sparse_path = quantum::schrod(
      sparse_h, zero(), pluss::make_series({0.0, std::numbers::pi}),
      krylov_options);
  expect(quantum::prob(sparse_path)
             .select(0, 1)
             .allclose(pluss::make_series({0.0, 1.0}), 2e-9, 2e-9),
         "sparse Schrodinger evolution disagrees with the dense solution");

  constexpr double rate = 0.7;
  const auto zero_h = quantum::op(pluss::series::zeros({2, 2}));
  quantum::CollapseFn varying_collapse = [rate](double time) {
    const double amplitude = std::sqrt(rate * (1.0 + time));
    return std::vector<quantum::Op>{quantum::op(
        cx({{0.0, 0.0}, {amplitude, 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}))};
  };
  const auto damp_times = pluss::make_series({0.0, 0.5, 1.0});
  const auto varying = quantum::lindblad([zero_h](double) { return zero_h; },
                                         quantum::density(one()), damp_times,
                                         varying_collapse, adaptive);
  for (std::int64_t index = 0; index < 3; ++index) {
    const double time = damp_times.select(0, index).item<double>();
    const double excited = varying.value()
                               .select(0, index)
                               .select(0, 1)
                               .select(0, 1)
                               .real()
                               .item<double>();
    expect(close(excited, std::exp(-rate * (time + 0.5 * time * time)), 2e-8),
           "time-dependent collapse evolution disagrees with its analytic "
           "survival law");
  }

  const auto sparse_zero =
      quantum::sparse_op(2, pluss::make_series(std::vector<std::int64_t>{0}),
                         pluss::make_series(std::vector<std::int64_t>{0}),
                         pluss::make_series({0.0}), {}, true);
  const auto sparse_collapse =
      quantum::sparse_op(2, pluss::make_series(std::vector<std::int64_t>{0}),
                         pluss::make_series(std::vector<std::int64_t>{1}),
                         pluss::make_series({std::sqrt(rate)}));
  const auto sparse_density =
      quantum::lindblad(sparse_zero, quantum::density(one()), damp_times,
                        {sparse_collapse}, adaptive);
  expect(close(sparse_density.value()
                   .select(0, 2)
                   .select(0, 1)
                   .select(0, 1)
                   .real()
                   .item<double>(),
               std::exp(-rate), 2e-8),
         "sparse Lindblad evolution disagrees with amplitude damping");

  quantum::SolveOptions implicit = adaptive;
  implicit.method = quantum::SolveMethod::ImplicitMidpoint;
  implicit.initial_step = 1.0;
  implicit.relative_tolerance = 2e-6;
  implicit.absolute_tolerance = 1e-9;
  implicit.linear.max_iterations = 8;
  implicit.linear.restart = 4;
  const auto stiff = quantum::lindblad(
      zero_h, quantum::density(one()), pluss::make_series({0.0, 1.0}),
      {quantum::op(
          cx({{0.0, 0.0}, {std::sqrt(20.0), 0.0}, {0.0, 0.0}, {0.0, 0.0}},
             {2, 2}))},
      implicit);
  expect(close(stiff.value()
                   .select(0, 1)
                   .select(0, 1)
                   .select(0, 1)
                   .real()
                   .item<double>(),
               std::exp(-20.0), 2e-6),
         "implicit Lindblad evolution failed on a stiff damping mode");
}

void test_failures() {
  bool nonhermitian = false;
  try {
    (void)quantum::spectrum(
        quantum::op(pluss::make_series({{0.0, 1.0}, {0.0, 0.0}})));
  } catch (const std::domain_error &) {
    nonhermitian = true;
  }
  expect(nonhermitian, "spectrum accepted a non-Hermitian operator");

  bool bad_times = false;
  try {
    (void)quantum::schrod(pauli_x(), zero(),
                          pluss::make_series({0.0, 1.0, 1.0}));
  } catch (const std::invalid_argument &) {
    bad_times = true;
  }
  expect(bad_times, "Schrodinger solver accepted repeated times");

  bool bad_density = false;
  try {
    (void)quantum::entropy(
        quantum::op(pluss::make_series({{1.2, 0.0}, {0.0, -0.2}})));
  } catch (const std::domain_error &) {
    bad_density = true;
  }
  expect(bad_density, "entropy accepted a non-positive density operator");

  bool false_sparse_contract = false;
  try {
    (void)quantum::sparse_op(2,
                             pluss::make_series(std::vector<std::int64_t>{0}),
                             pluss::make_series(std::vector<std::int64_t>{1}),
                             pluss::make_series({1.0}), {}, true);
  } catch (const std::domain_error &) {
    false_sparse_contract = true;
  }
  expect(false_sparse_contract,
         "sparse_op accepted a false Hermitian contract");
}

} // namespace

int main() {
  try {
    test_series_complex_eigh();
    test_series_eigh_reverse();
    test_spectrum_entropy_and_fidelity();
    test_unitary_and_schrodinger();
    test_kraus_channel();
    test_lindblad_amplitude_damping();
    test_time_dependent_and_sparse_dynamics();
    test_failures();
    std::cout
        << "NatSci quantum spectra, channels, and dynamics checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
