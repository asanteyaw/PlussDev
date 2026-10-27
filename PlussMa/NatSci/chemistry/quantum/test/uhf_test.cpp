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

void test_closed_shell_parity() {
  quantum::HfOptions options;
  options.energy_tolerance = 1e-12;
  options.gradient_tolerance = 1e-11;
  const auto integrals = two_orbital_integrals();
  const auto restricted = quantum::rhf(integrals, 2, options);
  const auto unrestricted = quantum::uhf(integrals, 1, 1, options);

  expect(unrestricted.converged.item<bool>() &&
             unrestricted.total_energy.allclose(restricted.total_energy, 2e-10,
                                                2e-10),
         "closed-shell UHF did not reproduce RHF energy");
  expect(unrestricted.alpha_density.add(unrestricted.beta_density)
             .allclose(restricted.density, 2e-9, 2e-9),
         "closed-shell UHF spin densities did not reproduce RHF density");
  expect(unrestricted.alpha_fock.allclose(restricted.fock, 2e-9, 2e-9) &&
             unrestricted.beta_fock.allclose(restricted.fock, 2e-9, 2e-9) &&
             close(unrestricted.spin_squared.item<double>(), 0.0, 2e-10),
         "closed-shell UHF lost Fock or singlet-spin parity");
}

void test_one_electron_open_shell() {
  quantum::HfOptions options;
  options.energy_tolerance = 1e-12;
  options.gradient_tolerance = 1e-11;
  const auto result = quantum::uhf(two_orbital_integrals(), 1, 0, options);

  // det(H - e S) = 0.96 e^2 + 1.5 e + 0.4175.
  const double orbital = (-1.5 - std::sqrt(0.6468)) / 1.92;
  expect(result.converged.item<bool>() &&
             close(result.total_energy.item<double>(), orbital + 0.7, 2e-10),
         "one-electron UHF disagrees with the generalized eigenvalue oracle");
  expect(result.beta_density.eq(0.0).all().item<bool>() &&
             close(result.spin_squared.item<double>(), 0.75, 2e-10),
         "one-electron UHF lost its empty beta channel or doublet spin");
  const auto population = result.alpha_density.transpose(-2, -1)
                              .mul(two_orbital_integrals().overlap)
                              .sum(-1, false)
                              .sum(-1, false);
  expect(close(population.item<double>(), 1.0, 2e-10),
         "one-electron UHF density has the wrong metric trace");

  const auto restarted = quantum::uhf(
      two_orbital_integrals(), 1, 0,
      quantum::UhfGuess{result.alpha_density, result.beta_density}, options);
  expect(restarted.converged.item<bool>() && restarted.iterations <= 2 &&
             restarted.total_energy.allclose(result.total_energy, 2e-11, 2e-11),
         "UHF density restart did not retain the open-shell solution");

  quantum::HfOptions fixed_point = options;
  fixed_point.diis_space = 0;
  const auto unaccelerated =
      quantum::uhf(two_orbital_integrals(), 1, 0, fixed_point);
  expect(unaccelerated.converged.item<bool>() &&
             unaccelerated.diis_steps == 0 &&
             unaccelerated.total_energy.allclose(result.total_energy, 2e-11,
                                                 2e-11),
         "DIIS and fixed-point UHF disagree for the one-electron oracle");
}

void test_batched_joint_diis() {
  const auto base = two_orbital_integrals();
  quantum::AoIntegrals batch{
      base.overlap.unsqueeze(0).expand({2, 2, 2}, true),
      base.core.unsqueeze(0).expand({2, 2, 2}, true),
      base.eri.unsqueeze(0).expand({2, 2, 2, 2, 2}, true),
      pluss::series::make(std::vector<double>{0.7, 1.2}),
  };
  quantum::HfOptions options;
  options.energy_tolerance = 1e-12;
  options.gradient_tolerance = 1e-11;
  options.diis_space = 3;
  const auto result = quantum::uhf(batch, 1, 1, options);

  expect(result.total_energy.sizes() == std::vector<std::int64_t>({2}) &&
             result.alpha_density.sizes() ==
                 std::vector<std::int64_t>({2, 2, 2}) &&
             result.converged.all().item<bool>() && result.diis_steps > 0,
         "batched UHF did not preserve lanes or use joint DIIS");
  expect(close(result.total_energy.select(0, 1).item<double>() -
                   result.total_energy.select(0, 0).item<double>(),
               0.5, 2e-10),
         "batched UHF did not preserve lane-wise nuclear offsets");
}

void test_one_orbital_reverse() {
  auto overlap = pluss::series::make(std::vector<double>{1.0}, {1, 1});
  auto core = pluss::series::make(std::vector<double>{-1.25}, {1, 1});
  auto eri = pluss::series::make(std::vector<double>{0.7}, {1, 1, 1, 1});
  auto nuclear = pluss::series::make(0.5);
  overlap.tracks_slope_(true);
  core.tracks_slope_(true);
  eri.tracks_slope_(true);
  nuclear.tracks_slope_(true);

  const auto result = quantum::uhf({overlap, core, eri, nuclear}, 1, 0);
  expect(close(result.total_energy.item<double>(), -0.75, 1e-12) &&
             close(result.spin_squared.item<double>(), 0.75, 1e-12),
         "analytic one-orbital UHF value is incorrect");
  result.total_energy.reverse();
  expect(close(core.sensitivity().host<double>().front(), 1.0, 1e-10) &&
             close(overlap.sensitivity().host<double>().front(), 1.25, 2e-9) &&
             close(eri.sensitivity().host<double>().front(), 0.0, 2e-9) &&
             close(nuclear.sensitivity().item<double>(), 1.0, 1e-10),
         "one-orbital UHF reverse sensitivities disagree with the analytic energy");
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
         "the UHF fixture must exercise strided storage");

  quantum::HfOptions options;
  options.energy_tolerance = 2e-6;
  options.gradient_tolerance = 2e-5;
  const auto result = quantum::uhf(strided, 1, 0, options);
  const double orbital = (-1.5 - std::sqrt(0.6468)) / 1.92;
  expect(result.converged.item<bool>() &&
             result.total_energy.scalar_type() == pluss::ScalarType::Float32 &&
             close(result.total_energy.item<float>(), orbital + 0.7, 4e-5),
         "Float32 strided UHF lost dtype, convergence, or oracle parity");
}

void test_contract_failures() {
  const auto valid = two_orbital_integrals();
  bool empty = false;
  try {
    (void)quantum::uhf(valid, 0, 0);
  } catch (const std::invalid_argument &) {
    empty = true;
  }
  expect(empty, "UHF accepted an empty determinant");

  bool overfilled = false;
  try {
    (void)quantum::uhf(valid, 3, 0);
  } catch (const std::invalid_argument &) {
    overfilled = true;
  }
  expect(overfilled, "UHF accepted more same-spin electrons than orbitals");

  bool bad_guess = false;
  try {
    (void)quantum::uhf(
        valid, 1, 0,
        quantum::UhfGuess{pluss::series::make({{1.0, 0.5}, {0.0, 0.0}}),
                          pluss::series::zeros({2, 2})});
  } catch (const std::invalid_argument &) {
    bad_guess = true;
  }
  expect(bad_guess, "UHF accepted an asymmetric restart density");
}

} // namespace

int main() {
  test_closed_shell_parity();
  test_one_electron_open_shell();
  test_batched_joint_diis();
  test_one_orbital_reverse();
  test_float32_strided_inputs();
  test_contract_failures();
  return 0;
}
