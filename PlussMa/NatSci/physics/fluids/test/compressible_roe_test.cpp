#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
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

common::ScalarQuantity<common::Length> metres(double value) {
  return common::ScalarQuantity<common::Length>::from_si(value);
}

common::ScalarQuantity<common::Time> seconds(double value) {
  return common::ScalarQuantity<common::Time>::from_si(value);
}

spatial::PeriodicGrid periodic1(std::int64_t cells, double width) {
  return spatial::make_periodic_grid({cells}, {metres(width)});
}

spatial::BoxGrid box1(std::int64_t cells, double width) {
  return spatial::make_box_grid({cells}, {metres(width)});
}

spatial::PeriodicGrid periodic2(std::int64_t rows, std::int64_t columns,
                                double width) {
  return spatial::make_periodic_grid({rows, columns},
                                     {metres(width), metres(width)});
}

spatial::BoxGrid box2(std::int64_t rows, std::int64_t columns, double width) {
  return spatial::make_box_grid({rows, columns},
                                {metres(width), metres(width)});
}

gas::State primitive(const Series &rho, const Series &velocity,
                     const Series &pressure,
                     const spatial::PeriodicGrid &grid) {
  const std::int64_t component = rho.dim() - grid.dim();
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series energy = pressure.div(0.4).add(
      velocity.square().sum(component, false).mul(rho).mul(0.5));
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(energy),
                   grid);
}

gas::State primitive(const Series &rho, const Series &velocity,
                     const Series &pressure, const spatial::BoxGrid &grid) {
  const std::int64_t component = rho.dim() - grid.dim();
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series energy = pressure.div(0.4).add(
      velocity.square().sum(component, false).mul(rho).mul(0.5));
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(energy),
                   grid);
}

gas::detail::Raw scalar_raw(double rho, double speed, double pressure) {
  const Series density = pluss::series::full({}, rho, ScalarType::Float64);
  const Series velocity = pluss::series::full({1}, speed, ScalarType::Float64);
  return {density, velocity.mul(rho),
          pluss::series::full({}, pressure / 0.4 + 0.5 * rho * speed * speed,
                              ScalarType::Float64)};
}

gas::detail::Raw scalar_raw(double rho, double normal, double tangent,
                            double pressure) {
  const Series density = pluss::series::full({}, rho, ScalarType::Float64);
  const Series velocity =
      pluss::series::make({normal, tangent}, ScalarType::Float64);
  return {
      density, velocity.mul(rho),
      pluss::series::full({},
                          pressure / 0.4 +
                              0.5 * rho * (normal * normal + tangent * tangent),
                          ScalarType::Float64)};
}

double entropy_abs(double value, double left, double right, double scale,
                   double coefficient) {
  const double width =
      std::max({0.0, value - left, right - value, coefficient * scale});
  if (width > 0.0 && std::abs(value) < width)
    return (value * value + width * width) / (2.0 * width);
  return std::abs(value);
}

std::array<double, 3> roe_reference(double rho_l, double u_l, double p_l,
                                    double rho_r, double u_r, double p_r,
                                    double entropy) {
  constexpr double gamma = 1.4;
  const double e_l = p_l / (gamma - 1.0) + 0.5 * rho_l * u_l * u_l;
  const double e_r = p_r / (gamma - 1.0) + 0.5 * rho_r * u_r * u_r;
  const double h_l = (e_l + p_l) / rho_l;
  const double h_r = (e_r + p_r) / rho_r;
  const double root_l = std::sqrt(rho_l);
  const double root_r = std::sqrt(rho_r);
  const double root_sum = root_l + root_r;
  const double velocity = (root_l * u_l + root_r * u_r) / root_sum;
  const double enthalpy = (root_l * h_l + root_r * h_r) / root_sum;
  const double sound2 = (gamma - 1.0) * (enthalpy - 0.5 * velocity * velocity);
  const double sound = std::sqrt(sound2);
  const double density = root_l * root_r;
  const double pressure_wave = (p_r - p_l) / sound2;
  const double acoustic = density * (u_r - u_l) / sound;
  const double alpha0 = rho_r - rho_l - pressure_wave;
  const double alpha_minus = 0.5 * (pressure_wave - acoustic);
  const double alpha_plus = 0.5 * (pressure_wave + acoustic);
  const double scale = std::abs(velocity) + sound;
  const double lambda0 = std::abs(velocity);
  const double lambda_minus =
      entropy_abs(velocity - sound, u_l - std::sqrt(gamma * p_l / rho_l),
                  u_r - std::sqrt(gamma * p_r / rho_r), scale, entropy);
  const double lambda_plus =
      entropy_abs(velocity + sound, u_l + std::sqrt(gamma * p_l / rho_l),
                  u_r + std::sqrt(gamma * p_r / rho_r), scale, entropy);
  const double d0 = lambda0 * alpha0;
  const double dm = lambda_minus * alpha_minus;
  const double dp = lambda_plus * alpha_plus;
  return {
      0.5 * (rho_l * u_l + rho_r * u_r - d0 - dm - dp),
      0.5 * (rho_l * u_l * u_l + p_l + rho_r * u_r * u_r + p_r - d0 * velocity -
             dm * (velocity - sound) - dp * (velocity + sound)),
      0.5 *
          ((e_l + p_l) * u_l + (e_r + p_r) * u_r -
           d0 * 0.5 * velocity * velocity - dm * (enthalpy - velocity * sound) -
           dp * (enthalpy + velocity * sound))};
}

void test_reference_flux_and_contact() {
  const auto left = scalar_raw(1.0, 0.75, 1.0);
  const auto right = scalar_raw(0.6, -0.2, 0.55);
  gas::RoePolicy policy;
  policy.entropy = 0.08;
  const auto flux =
      gas::detail::roe_flux(left, right, 0, 0, gas::Model{}, policy);
  const auto expected =
      roe_reference(1.0, 0.75, 1.0, 0.6, -0.2, 0.55, policy.entropy);
  expect(std::abs(flux.mass.item<double>() - expected[0]) < 2e-13 &&
             std::abs(flux.momentum.item<double>() - expected[1]) < 2e-13 &&
             std::abs(flux.energy.item<double>() - expected[2]) < 2e-13,
         "Roe flux disagrees with an independently reconstructed 1-D oracle");

  constexpr std::int64_t n = 32;
  const auto grid = periodic1(n, 1.0 / n);
  const Series lane = pluss::series::arange(0, n, 1, ScalarType::Float64);
  const Series rho = lane.lt(n / 2).where(pluss::series::ones_like(lane),
                                          pluss::series::full_like(lane, 2.0));
  const auto state = primitive(rho, pluss::series::zeros({1, n}),
                               pluss::series::ones_like(rho), grid);
  gas::Options options;
  options.recon = gas::Recon::Cell;
  options.flux = gas::FluxScheme::Roe;
  const auto change = gas::rate(state, grid, {}, options);
  expect(change.density.si_value().abs().max().item<double>() < 2e-13 &&
             change.momentum.si_value().abs().max().item<double>() < 2e-13 &&
             change.energy.si_value().abs().max().item<double>() < 2e-13,
         "Roe did not preserve a stationary contact exactly");

  const auto shear_left = scalar_raw(1.0, 0.3, 0.7, 1.0);
  const auto shear_right = scalar_raw(1.0, 0.3, -0.4, 1.0);
  const auto shear_flux = gas::detail::roe_flux(shear_left, shear_right, 0, 0,
                                                gas::Model{}, policy);
  const auto upwind =
      gas::detail::physical_flux(shear_left, 0, 0, gas::Model{});
  expect(shear_flux.mass.allclose(upwind.mass, 2e-13, 2e-13) &&
             shear_flux.momentum.allclose(upwind.momentum, 2e-13, 2e-13) &&
             shear_flux.energy.allclose(upwind.energy, 2e-13, 2e-13),
         "Roe did not exactly upwind a multidimensional shear wave");
}

void test_entropy_and_low_mach_policies() {
  const Series zero = pluss::series::full({}, 0.0, ScalarType::Float64);
  const Series minus = pluss::series::full({}, -1.0, ScalarType::Float64);
  const Series plus = pluss::series::full({}, 1.0, ScalarType::Float64);
  const Series scale = pluss::series::full({}, 1.0, ScalarType::Float64);
  expect(
      std::abs(
          gas::detail::roe_abs(zero, minus, plus, scale, 0.0).item<double>() -
          0.5) < 1e-15,
      "Roe transonic entropy correction has the wrong sonic limit");

  const auto left = scalar_raw(1.0, -1e-3, 1.0);
  const auto right = scalar_raw(1.0, 1e-3, 1.0);
  gas::RoePolicy classic;
  classic.entropy = 0.0;
  gas::RoePolicy low = classic;
  low.mach = gas::RoeMach::Rieper;
  low.mach_floor = 0.05;
  const auto classic_flux =
      gas::detail::roe_flux(left, right, 0, 0, gas::Model{}, classic);
  const auto low_flux =
      gas::detail::roe_flux(left, right, 0, 0, gas::Model{}, low);
  constexpr double centered_momentum_flux = 1.0 + 1e-6;
  const double classic_loss =
      centered_momentum_flux - classic_flux.momentum.item<double>();
  const double low_loss =
      centered_momentum_flux - low_flux.momentum.item<double>();
  expect(classic_loss > 0.0 && std::abs(low_loss / classic_loss - 0.05) < 2e-11,
         "Rieper policy did not scale the low-Mach normal-velocity "
         "dissipation: classic=" +
             std::to_string(classic_loss) +
             ", low=" + std::to_string(low_loss));
}

void test_periodic_bounded_batch_and_reverse() {
  constexpr std::int64_t n = 24;
  const auto periodic = periodic1(n, 1.0 / n);
  const Series rho = pluss::series::full({2, n}, 1.2, ScalarType::Float32);
  const Series velocity =
      pluss::series::full({2, 1, n}, 0.125, ScalarType::Float32);
  const Series pressure = pluss::series::full_like(rho, 0.9);
  const auto state = primitive(rho, velocity, pressure, periodic);
  gas::Options options;
  options.recon = gas::Recon::Weno5;
  options.flux = gas::FluxScheme::Roe;
  options.roe.mach = gas::RoeMach::Rieper;
  const auto change = gas::rate(state, periodic, {}, options);
  expect(change.density.si_value().scalar_type() == ScalarType::Float32 &&
             change.density.si_value().sizes() == rho.sizes() &&
             change.density.si_value().abs().max().item<float>() < 2e-5f &&
             change.momentum.si_value().abs().max().item<float>() < 2e-5f &&
             change.energy.si_value().abs().max().item<float>() < 2e-5f,
         "Roe did not preserve a batched Float32 periodic free stream");

  const auto box = box1(n, 1.0 / n);
  const auto boxed = primitive(rho, velocity, pressure, box);
  const auto prescribed =
      gas::primitive(common::SeriesQuantity<gas::Density>::from_si(
                         pluss::series::full({}, 1.2, ScalarType::Float32)),
                     common::SeriesQuantity<common::Speed>::from_si(
                         pluss::series::full({1}, 0.125, ScalarType::Float32)),
                     common::SeriesQuantity<common::Pressure>::from_si(
                         pluss::series::full({}, 0.9, ScalarType::Float32)));
  const auto rules =
      gas::bounds(box, {gas::fixed({0, spatial::Side::Low}, prescribed),
                        gas::fixed({0, spatial::Side::High}, prescribed)});
  const auto bounded = gas::rate(boxed, box, rules, {}, options);
  expect(bounded.density.si_value().abs().max().item<float>() < 2e-5f &&
             bounded.momentum.si_value().abs().max().item<float>() < 2e-5f &&
             bounded.energy.si_value().abs().max().item<float>() < 2e-5f,
         "bounded Roe disturbed a matching batched free stream");

  Series amplitude = pluss::series::full({}, 0.03, ScalarType::Float64);
  amplitude.tracks_slope_(true);
  const Series x = pluss::series::arange(0, n, 1, ScalarType::Float64)
                       .mul(6.283185307179586 / n);
  const Series wave = x.sin();
  const auto tracked =
      primitive(wave.mul(amplitude).add(1.1), wave.mul(0.04).unsqueeze(0),
                wave.mul(0.02).add(1.0), periodic);
  gas::rate(tracked, periodic, {}, options)
      .energy.si_value()
      .square()
      .sum()
      .reverse();
  expect(amplitude.has_sensitivity() &&
             std::isfinite(amplitude.sensitivity().item<double>()),
         "Roe flux severed reverse sensitivity");
}

void test_strong_jump_and_validation() {
  constexpr std::int64_t n = 128;
  const auto grid = periodic1(n, 1.0 / n);
  const Series lane = pluss::series::arange(0, n, 1, ScalarType::Float64);
  const Series side = lane.lt(n / 2);
  const Series rho = side.where(pluss::series::ones_like(lane),
                                pluss::series::full_like(lane, 0.125));
  const Series pressure = side.where(pluss::series::ones_like(lane),
                                     pluss::series::full_like(lane, 0.1));
  const auto state =
      primitive(rho, pluss::series::zeros({1, n}), pressure, grid);
  gas::Options options;
  options.rule = gas::Rule::Rk3;
  options.recon = gas::Recon::Weno5;
  options.flux = gas::FluxScheme::Roe;
  const auto next = gas::step(state, grid, seconds(2e-4), {}, options).state;
  const Series next_pressure = gas::pressure(next, grid).si_value();
  expect(next.density()
                 .si_value()
                 .gt(0.0)
                 .logical_and(next.density().si_value().isfinite())
                 .all()
                 .item<bool>() &&
             next_pressure.gt(0.0)
                 .logical_and(next_pressure.isfinite())
                 .all()
                 .item<bool>(),
         "Roe/WENO lost admissibility across a strong pressure jump");

  expect_failure(
      [&] {
        auto invalid = options;
        invalid.roe.entropy = -0.1;
        (void)gas::rate(state, grid, {}, invalid);
      },
      "negative Roe entropy width was accepted");
  expect_failure(
      [&] {
        auto invalid = options;
        invalid.roe.mach = static_cast<gas::RoeMach>(99);
        (void)gas::rate(state, grid, {}, invalid);
      },
      "an unknown Roe Mach policy was accepted");
  expect_failure(
      [&] {
        auto invalid = options;
        invalid.roe.mach_floor = 0.0;
        (void)gas::rate(state, grid, {}, invalid);
      },
      "a zero Roe Mach floor was accepted");
}

void test_batch_lane_admissibility_fallback() {
  constexpr std::int64_t n = 6;
  const auto grid = periodic1(n, 1.0 / n);
  const Series rho = pluss::series::ones({2, n}, ScalarType::Float64);
  const auto state = primitive(rho, pluss::series::zeros({2, 1, n}),
                               pluss::series::ones_like(rho), grid);
  const gas::detail::Raw raw = gas::Access::raw(state);
  const Series rho_rate = pluss::series::make(
      {{-2.0, 2.0, 0.0, 0.0, 0.0, 0.0}, {0.5, -0.5, 0.0, 0.0, 0.0, 0.0}},
      ScalarType::Float64);
  const gas::detail::Raw high{rho_rate, pluss::series::zeros_like(raw.mom),
                              pluss::series::zeros_like(raw.energy)};
  const gas::detail::Raw zero{pluss::series::zeros_like(raw.rho),
                              pluss::series::zeros_like(raw.mom),
                              pluss::series::zeros_like(raw.energy)};
  const auto next = gas::detail::admissible_advance(
      raw, high, 1.0, grid, gas::Model{}, [&] { return zero; },
      "Roe fallback test");
  const Series limited_lane = next.rho.select(0, 0);
  expect(limited_lane.gt(gas::Model{}.rho_floor).all().item<bool>() &&
             limited_lane.select(0, 0).item<double>() < 2e-12 &&
             std::abs(limited_lane.sum().item<double>() - n) < 2e-13 &&
             next.rho.select(0, 1).equal(pluss::series::make(
                 {1.5, 0.5, 1.0, 1.0, 1.0, 1.0}, ScalarType::Float64)),
         "admissibility limiter did not retain the largest valid fraction or "
         "isolate the failing batch lane conservatively");

  expect_failure(
      [&] {
        (void)gas::detail::admissible_advance(
            raw, high, 1.0, grid, gas::Model{}, [&] { return high; },
            "Roe fallback test");
      },
      "an inadmissible first-order fallback did not fail closed");

  const gas::detail::Raw nonfinite{
      pluss::series::full_like(raw.rho,
                               std::numeric_limits<double>::quiet_NaN()),
      pluss::series::zeros_like(raw.mom),
      pluss::series::zeros_like(raw.energy)};
  const auto recovered = gas::detail::admissible_advance(
      raw, nonfinite, 1.0, grid, gas::Model{}, [&] { return zero; },
      "Roe non-finite fallback test");
  expect(recovered.rho.equal(raw.rho) && recovered.mom.equal(raw.mom) &&
             recovered.energy.equal(raw.energy),
         "a zero limiter coefficient did not select the finite endpoint "
         "exactly");
}

void test_reconstruction_scaling() {
  constexpr std::int64_t n = 8;
  const auto grid = periodic1(n, 1.0 / n);
  const auto state =
      primitive(pluss::series::ones({n}, ScalarType::Float64),
                pluss::series::zeros({1, n}),
                pluss::series::ones({n}, ScalarType::Float64), grid);
  const gas::detail::Raw mean = gas::Access::raw(state);
  gas::detail::Raw candidate = mean;
  candidate.rho = pluss::series::full({n}, -1.0, ScalarType::Float64);
  const auto limited =
      gas::detail::limit_toward(mean, candidate, grid, gas::Model{});
  expect(gas::detail::valid(limited, grid, gas::Model{}).all().item<bool>() &&
             limited.rho.max().item<double>() < 2e-12,
         "reconstruction scaling did not retain the largest admissible "
         "fraction toward an invalid face state");
}

void test_h_correction() {
  const auto stencil_grid = periodic2(3, 3, 1.0);
  const std::vector<Series> eta{
      pluss::series::zeros({3, 3}, ScalarType::Float64),
      pluss::series::make({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}},
                          ScalarType::Float64)};
  const Series expected = pluss::series::make(
      {{6.0, 5.0, 6.0}, {9.0, 8.0, 9.0}, {9.0, 8.0, 9.0}}, ScalarType::Float64);
  expect(gas::detail::h_floor(eta, stencil_grid, 0).equal(expected),
         "periodic H-correction did not take the four transverse acoustic "
         "jumps around each normal face");

  const auto contact_left = scalar_raw(1.0, 0.0, 1.0);
  const auto contact_right = scalar_raw(2.0, 0.0, 1.0);
  gas::RoePolicy policy;
  const Series floor = pluss::series::full({}, 0.5, ScalarType::Float64);
  const auto corrected = gas::detail::roe_flux(contact_left, contact_right, 0,
                                               0, gas::Model{}, policy, &floor);
  expect(std::abs(corrected.mass.item<double>() + 0.25) < 2e-13,
         "H-correction did not floor the linearly degenerate Roe field");

  constexpr std::int64_t n = 16;
  const auto grid = periodic2(n, n, 1.0 / n);
  const Series row =
      pluss::series::arange(0, n, 1, ScalarType::Float64).unsqueeze(1);
  const Series checker = row.fmod(2.0).eq(0.0).where(
      pluss::series::ones({n, n}, ScalarType::Float64),
      pluss::series::full({n, n}, 2.0, ScalarType::Float64));
  const auto state =
      primitive(checker, pluss::series::zeros({2, n, n}),
                pluss::series::ones({n, n}, ScalarType::Float64), grid);
  gas::Options classic;
  classic.recon = gas::Recon::Cell;
  classic.flux = gas::FluxScheme::Roe;
  auto h = classic;
  h.roe.shock = gas::RoeShock::HFix;
  const double classic_change = gas::rate(state, grid, {}, classic)
                                    .density.si_value()
                                    .abs()
                                    .max()
                                    .item<double>();
  const double corrected_change = gas::rate(state, grid, {}, h)
                                      .density.si_value()
                                      .abs()
                                      .max()
                                      .item<double>();
  expect(classic_change < 2e-13 && corrected_change > 1.0,
         "H-correction did not suppress an odd-even grid-aligned contact mode");

  const auto bounded_grid = box2(n, n, 1.0 / n);
  const Series rho = pluss::series::full({n, n}, 1.2, ScalarType::Float64);
  const Series velocity =
      pluss::series::full({2, n, n}, 0.125, ScalarType::Float64);
  const Series pressure = pluss::series::full_like(rho, 0.9);
  const auto bounded_state = primitive(rho, velocity, pressure, bounded_grid);
  const auto prescribed =
      gas::primitive(common::SeriesQuantity<gas::Density>::from_si(
                         pluss::series::full({}, 1.2, ScalarType::Float64)),
                     common::SeriesQuantity<common::Speed>::from_si(
                         pluss::series::full({2}, 0.125, ScalarType::Float64)),
                     common::SeriesQuantity<common::Pressure>::from_si(
                         pluss::series::full({}, 0.9, ScalarType::Float64)));
  const auto rules = gas::bounds(
      bounded_grid, {gas::fixed({0, spatial::Side::Low}, prescribed),
                     gas::fixed({0, spatial::Side::High}, prescribed),
                     gas::fixed({1, spatial::Side::Low}, prescribed),
                     gas::fixed({1, spatial::Side::High}, prescribed)});
  const auto bounded_change =
      gas::rate(bounded_state, bounded_grid, rules, {}, h);
  expect(bounded_change.density.si_value().abs().max().item<double>() < 2e-12 &&
             bounded_change.momentum.si_value().abs().max().item<double>() <
                 2e-12 &&
             bounded_change.energy.si_value().abs().max().item<double>() <
                 2e-12,
         "bounded H-correction disturbed a matching free stream");

  Series amplitude = pluss::series::full({}, 0.1, ScalarType::Float64);
  amplitude.tracks_slope_(true);
  const Series signed_rows = row.fmod(2.0).eq(0.0).where(
      pluss::series::ones({n, n}, ScalarType::Float64),
      pluss::series::full({n, n}, -1.0, ScalarType::Float64));
  const auto tracked = primitive(
      signed_rows.mul(amplitude).add(1.5), pluss::series::zeros({2, n, n}),
      pluss::series::ones({n, n}, ScalarType::Float64), grid);
  gas::rate(tracked, grid, {}, h).density.si_value().square().sum().reverse();
  expect(amplitude.has_sensitivity() &&
             std::isfinite(amplitude.sensitivity().item<double>()),
         "H-correction severed reverse sensitivity");

  expect_failure(
      [&] {
        auto invalid = h;
        invalid.roe.shock = static_cast<gas::RoeShock>(99);
        (void)gas::rate(state, grid, {}, invalid);
      },
      "an unknown Roe shock policy was accepted");
}

} // namespace

int main() {
  try {
    test_reference_flux_and_contact();
    test_entropy_and_low_mach_policies();
    test_periodic_bounded_batch_and_reverse();
    test_strong_jump_and_validation();
    test_batch_lane_admissibility_fallback();
    test_reconstruction_scaling();
    test_h_correction();
    std::cout << "NatSci Roe-flux checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
