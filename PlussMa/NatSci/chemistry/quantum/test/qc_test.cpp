#include "pluss/codem/series_factory.h"
#include "pluss/subsp/natsci/chemistry/quantum.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace quantum = pluss::subsp::natsci::chemistry::quantum;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool close(double left, double right, double tolerance = 1e-9) {
  return std::abs(left - right) <=
         tolerance * std::max({1.0, std::abs(left), std::abs(right)});
}

quantum::AoIntegrals two_orbital_integrals() {
  return {
      pluss::series::make({{1.0, 0.2}, {0.2, 1.0}}),
      pluss::series::make({{-1.2, -0.25}, {-0.25, -0.4}}),
      pluss::series::make(std::vector<double>{0.53, 0.06, 0.06, 0.29, 0.06,
                                              0.0125, 0.0125, 0.01, 0.06,
                                              0.0125, 0.0125, 0.01, 0.29, 0.01,
                                              0.01, 0.25},
                          {2, 2, 2, 2}),
      pluss::series::make(0.7),
  };
}

void test_common_quantum_facade() {
  const auto orbital =
      quantum::normalize(quantum::ket(pluss::series::make({1.0, 1.0})));
  const auto projector = quantum::density(orbital);
  const auto hamiltonian =
      quantum::op(pluss::series::make({{-1.0, 0.0}, {0.0, 2.0}}));
  const auto spectrum = quantum::spectrum(hamiltonian);
  expect(close(quantum::trace(projector).item<double>(), 1.0, 1e-12),
         "chemistry facade lost normalized density semantics");
  expect(
      spectrum.values.allclose(pluss::series::make({-1.0, 2.0}), 1e-12, 1e-12),
      "chemistry facade lost the common eigensystem");
  expect(close(quantum::entropy(projector).item<double>(), 0.0, 1e-12),
         "chemistry facade lost pure-state entropy semantics");
}

void test_two_orbital_rhf() {
  quantum::HfOptions options;
  options.energy_tolerance = 1e-12;
  options.gradient_tolerance = 1e-11;
  const auto result = quantum::rhf(two_orbital_integrals(), 2, options);

  expect(result.converged.item<bool>() && result.diis_steps > 0 &&
             result.gradient_rms.item<double>() <= options.gradient_tolerance,
         "two-orbital RHF did not converge through DIIS");
  expect(close(result.total_energy.item<double>(), -1.1803431924170917, 2e-10),
         "RHF energy disagrees with the independent SCF oracle");
  expect(result.orbital_energy.allclose(
             pluss::series::make({-0.6853092345584817, 0.2211087844218902}),
             2e-9, 2e-9),
         "RHF orbital energies disagree with the independent SCF oracle");
  expect(result.density.allclose(
             pluss::series::make({{1.9111831594876285, 0.1797687885661101},
                                  {0.1797687885661101, 0.016909325085927743}}),
             2e-9, 2e-9),
         "RHF density disagrees with the independent SCF oracle");

  const auto &integrals = two_orbital_integrals();
  const auto metric = result.coefficients.transpose(-2, -1)
                          .matmul(integrals.overlap)
                          .matmul(result.coefficients);
  expect(metric.allclose(pluss::series::eye(2, pluss::ScalarType::Float64),
                         2e-10, 2e-10),
         "RHF orbitals are not orthonormal in the AO overlap metric");

  quantum::HfOptions restart_options = options;
  restart_options.max_iterations = 4;
  const auto restarted =
      quantum::rhf(integrals, 2, result.density, restart_options);
  expect(restarted.converged.item<bool>() &&
             restarted.total_energy.allclose(result.total_energy, 2e-11, 2e-11),
         "RHF restart did not retain the converged solution");

  quantum::HfOptions fixed_point = options;
  fixed_point.diis_space = 0;
  const auto unaccelerated = quantum::rhf(integrals, 2, fixed_point);
  expect(unaccelerated.converged.item<bool>() &&
             unaccelerated.diis_steps == 0 &&
             unaccelerated.total_energy.allclose(result.total_energy, 2e-10,
                                                 2e-10),
         "DIIS and fixed-point RHF disagree at convergence");
}

void test_batched_broadcasting() {
  const auto base = two_orbital_integrals();
  quantum::AoIntegrals batch{
      base.overlap.unsqueeze(0).expand({1, 3, 2, 2}, true),
      base.core.unsqueeze(0).unsqueeze(0).expand({2, 1, 2, 2}, true),
      base.eri.unsqueeze(0).unsqueeze(0).expand({2, 1, 2, 2, 2, 2}, true),
      pluss::series::make(std::vector<double>{0.7, 1.2}, {2, 1}),
  };
  quantum::HfOptions options;
  options.energy_tolerance = 1e-12;
  options.gradient_tolerance = 1e-11;
  const auto result = quantum::rhf(batch, 2, options);
  expect(result.total_energy.sizes() == std::vector<std::int64_t>({2, 3}) &&
             result.density.sizes() == std::vector<std::int64_t>({2, 3, 2, 2}),
         "RHF did not preserve the broadcast batch shape");
  expect(result.converged.all().item<bool>(),
         "a broadcast RHF lane did not converge");
  expect(result.total_energy.select(0, 0).allclose(
             pluss::series::make({-1.1803431924170917, -1.1803431924170917,
                                  -1.1803431924170917}),
             2e-10, 2e-10) &&
             result.total_energy.select(0, 1).allclose(
                 pluss::series::make({-0.6803431924170917, -0.6803431924170917,
                                      -0.6803431924170917}),
                 2e-10, 2e-10),
         "RHF batch lanes did not retain their nuclear-energy offsets");
}

void test_independent_diis_batch_lanes() {
  const auto base = two_orbital_integrals();
  const auto shifted_core =
      base.core.add(pluss::series::make({{0.08, -0.03}, {-0.03, 0.04}}));
  const auto cores =
      base.core.unsqueeze(0).unite({shifted_core.unsqueeze(0)}, 0);
  quantum::AoIntegrals batch{
      base.overlap,
      cores,
      base.eri,
      pluss::series::make(std::vector<double>{0.7, 0.9}),
  };
  quantum::HfOptions options;
  options.energy_tolerance = 1e-11;
  options.gradient_tolerance = 1e-9;
  options.diis_space = 3;
  const auto together = quantum::rhf(batch, 2, options);

  auto first = base;
  auto second = base;
  second.core = shifted_core;
  second.nuclear_energy = pluss::series::make(0.9);
  const auto first_alone = quantum::rhf(first, 2, options);
  const auto second_alone = quantum::rhf(second, 2, options);
  expect(together.converged.all().item<bool>() && together.diis_steps > 0,
         "batched DIIS did not converge every independent lane");
  expect(close(together.total_energy.select(0, 0).item<double>(),
               first_alone.total_energy.item<double>(), 2e-10) &&
             close(together.total_energy.select(0, 1).item<double>(),
                   second_alone.total_energy.item<double>(), 2e-10) &&
             together.density.select(0, 0).allclose(first_alone.density, 2e-9,
                                                    2e-9) &&
             together.density.select(0, 1).allclose(second_alone.density, 2e-9,
                                                    2e-9),
         "batched DIIS coupled lanes or disagreed with separate solves");
}

void test_float32_strided_inputs() {
  const auto base = two_orbital_integrals();
  quantum::AoIntegrals strided{
      base.overlap.to(pluss::ScalarType::Float32).transpose(-2, -1),
      base.core.to(pluss::ScalarType::Float32).transpose(-2, -1),
      base.eri.to(pluss::ScalarType::Float32).permute({1, 0, 3, 2}),
      base.nuclear_energy.to(pluss::ScalarType::Float32),
  };
  expect(!strided.overlap.is_contiguous() && !strided.core.is_contiguous() &&
             !strided.eri.is_contiguous(),
         "the Float32 RHF fixture must exercise strided storage");

  quantum::HfOptions options;
  options.energy_tolerance = 2e-6;
  options.gradient_tolerance = 2e-5;
  const auto result = quantum::rhf(strided, 2, options);
  expect(
      result.converged.item<bool>() &&
          result.total_energy.scalar_type() == pluss::ScalarType::Float32 &&
          close(result.total_energy.item<float>(), -1.1803431924170917, 4e-5),
      "Float32 strided RHF lost dtype, convergence, or numerical parity");
}

void test_one_orbital_reverse() {
  auto overlap = pluss::series::make(std::vector<double>{1.0}, {1, 1});
  auto core = pluss::series::make(std::vector<double>{-1.25}, {1, 1});
  auto eri = pluss::series::make(std::vector<double>{0.7},
                                 std::vector<std::int64_t>{1, 1, 1, 1});
  auto nuclear = pluss::series::make(0.5);
  overlap.tracks_slope_(true);
  core.tracks_slope_(true);
  eri.tracks_slope_(true);
  nuclear.tracks_slope_(true);

  const auto result = quantum::rhf({overlap, core, eri, nuclear}, 2);
  expect(close(result.total_energy.item<double>(), -1.3, 1e-12) &&
             result.gradient_rms.item<double>() == 0.0,
         "one-orbital RHF energy or empty-virtual gradient is incorrect");
  result.total_energy.reverse();
  expect(close(core.sensitivity().item<double>(), 2.0, 1e-10) &&
             close(eri.sensitivity().item<double>(), 1.0, 1e-10) &&
             close(nuclear.sensitivity().item<double>(), 1.0, 1e-10) &&
             close(overlap.sensitivity().item<double>(), 1.1, 2e-9),
         "RHF reverse sensitivities disagree with the analytic solution");
}

void test_contract_failures() {
  const auto valid = two_orbital_integrals();
  bool odd = false;
  try {
    (void)quantum::rhf(valid, 1);
  } catch (const std::invalid_argument &) {
    odd = true;
  }
  expect(odd, "RHF accepted an odd electron count");

  bool singular = false;
  try {
    auto bad = valid;
    bad.overlap = pluss::series::make({{1.0, 1.0}, {1.0, 1.0}});
    (void)quantum::rhf(bad, 2);
  } catch (const std::invalid_argument &) {
    singular = true;
  }
  expect(singular, "RHF accepted a singular overlap metric");

  bool asymmetric = false;
  try {
    auto bad = valid;
    bad.eri = pluss::series::make(
        std::vector<double>{0.53, 0.06, 0.06, 0.29, 0.25, 0.0125, 0.0125, 0.01,
                            0.06, 0.0125, 0.0125, 0.01, 0.29, 0.01, 0.01, 0.25},
        {2, 2, 2, 2});
    (void)quantum::rhf(bad, 2);
  } catch (const std::invalid_argument &) {
    asymmetric = true;
  }
  expect(asymmetric, "RHF accepted an ERI tensor without pair symmetry");

  bool bad_diis_space = false;
  try {
    quantum::HfOptions options;
    options.diis_space = 1;
    (void)quantum::rhf(valid, 2, options);
  } catch (const std::invalid_argument &) {
    bad_diis_space = true;
  }
  expect(bad_diis_space, "RHF accepted a one-vector DIIS subspace");

  bool bad_diis_rcond = false;
  try {
    quantum::HfOptions options;
    options.diis_rcond = 0.0;
    (void)quantum::rhf(valid, 2, options);
  } catch (const std::invalid_argument &) {
    bad_diis_rcond = true;
  }
  expect(bad_diis_rcond, "RHF accepted a non-positive DIIS cutoff");

  quantum::HfOptions bounded;
  bounded.max_iterations = 1;
  const auto incomplete = quantum::rhf(valid, 2, bounded);
  expect(!incomplete.converged.item<bool>() && incomplete.iterations == 1 &&
             incomplete.diis_steps == 0,
         "RHF hid bounded non-convergence or reported a DIIS step too early");
}

} // namespace

int main() {
  test_common_quantum_facade();
  test_two_orbital_rhf();
  test_batched_broadcasting();
  test_independent_diis_batch_lanes();
  test_float32_strided_inputs();
  test_one_orbital_reverse();
  test_contract_failures();
  return 0;
}
