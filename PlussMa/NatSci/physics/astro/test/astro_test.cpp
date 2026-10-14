#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace astro = pluss::subsp::natsci::physics::astro;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool close(double actual, double expected, double tolerance = 1e-9) {
  const double scale = std::max({1.0, std::abs(actual), std::abs(expected)});
  return std::abs(actual - expected) <= tolerance * scale;
}

void expect_shape(const pluss::Series &value,
                  std::initializer_list<std::int64_t> expected,
                  const std::string &message) {
  expect(value.sizes() == std::vector<std::int64_t>(expected), message);
}

auto mu(double value) {
  return common::ScalarQuantity<astro::GravParam>::from_si(value);
}

auto dims(const pluss::Series &value) {
  return common::SeriesQuantity<common::Dimensionless>::from_si(value);
}

astro::Elements simple_elem(const pluss::Series &axis,
                            const pluss::Series &ecc,
                            const pluss::Series &tilt,
                            const pluss::Series &node,
                            const pluss::Series &peri,
                            const pluss::Series &phase) {
  return astro::make_elem(
      common::SeriesQuantity<common::Length>::from_si(axis), dims(ecc),
      dims(tilt), dims(node), dims(peri), dims(phase));
}

void test_vallado_state() {
  constexpr double km = 1000.0;
  const auto state = astro::make_state(
      common::SeriesQuantity<common::Length>::from_si(pluss::series::make(
          {6524.834 * km, 6862.875 * km, 6448.296 * km})),
      common::SeriesQuantity<common::Speed>::from_si(pluss::series::make(
          {4.901327 * km, 5.533756 * km, -1.976341 * km})));
  const auto elem = astro::to_elem(state, mu(398600.4418 * km * km * km));
  constexpr double deg = std::numbers::pi / 180.0;
  expect(close(elem.axis().si_value().item<double>(), 36127.3376 * km, 2e-7),
         "Vallado semimajor axis is incorrect");
  expect(close(elem.ecc().si_value().item<double>(), 0.8328534, 2e-7),
         "Vallado eccentricity is incorrect");
  expect(close(elem.tilt().si_value().item<double>(), 87.8691 * deg, 1e-6),
         "Vallado inclination is incorrect");
  expect(close(elem.node().si_value().item<double>(), 227.8983 * deg, 1e-6),
         "Vallado node longitude is incorrect");
  expect(close(elem.peri().si_value().item<double>(), 53.3849 * deg, 1e-6),
         "Vallado periapsis angle is incorrect");
  expect(close(elem.phase().si_value().item<double>(), 92.3352 * deg, 1e-6),
         "Vallado true anomaly is incorrect");

  const auto rebuilt = astro::from_elem(elem, mu(398600.4418 * km * km * km));
  expect(rebuilt.position().si_value().allclose(state.position().si_value(),
                                                2e-7, 1e-4),
         "Vallado position did not survive element round trip");
  expect(rebuilt.velocity().si_value().allclose(state.velocity().si_value(),
                                                2e-7, 1e-7),
         "Vallado velocity did not survive element round trip");
}

void test_singular_conventions() {
  const auto circular = astro::make_state(
      common::SeriesQuantity<common::Length>::from_si(
          pluss::series::make({0.0, 2.0, 0.0})),
      common::SeriesQuantity<common::Speed>::from_si(
          pluss::series::make({-std::sqrt(0.5), 0.0, 0.0})));
  const auto elem = astro::to_elem(circular, mu(1.0));
  expect(close(elem.axis().si_value().item<double>(), 2.0),
         "circular axis is incorrect");
  expect(elem.ecc().si_value().item<double>() < 1e-12,
         "circular eccentricity is not zero");
  expect(close(elem.node().si_value().item<double>(), 0.0) &&
             close(elem.peri().si_value().item<double>(), 0.0),
         "circular equatorial singular angles were not canonicalized");
  expect(close(elem.phase().si_value().item<double>(),
               std::numbers::pi / 2.0),
         "circular equatorial phase did not become true longitude");
  const auto rebuilt = astro::from_elem(elem, mu(1.0));
  expect(rebuilt.position().si_value().allclose(circular.position().si_value(),
                                                1e-11, 1e-11),
         "circular convention does not round trip");
}

void test_high_rank_batches() {
  const pluss::Series axis =
      pluss::series::make({2.0, 3.0}).reshape({2, 1, 1}).expand({2, 3, 4});
  const pluss::Series ecc = pluss::series::full_like(axis, 0.25);
  const pluss::Series tilt = pluss::series::full_like(axis, 0.4);
  const pluss::Series node = pluss::series::full_like(axis, 0.7);
  const pluss::Series peri = pluss::series::full_like(axis, 1.1);
  const pluss::Series phase =
      pluss::series::arange(0, 24, 1, pluss::ScalarType::Float64)
          .reshape({2, 3, 4})
          .mul(0.03);
  const auto elem = simple_elem(axis, ecc, tilt, node, peri, phase);
  const auto state = astro::from_elem(elem, mu(7.0));
  expect_shape(state.position().si_value(), {2, 3, 4, 3},
               "rank-four Cartesian output changed batch shape");
  const auto back = astro::to_elem(state, mu(7.0));
  expect(back.axis().si_value().allclose(axis, 2e-10, 2e-10),
         "rank-three element field did not round trip");
  expect(back.ecc().si_value().allclose(ecc, 2e-10, 2e-10),
         "batched eccentricity did not round trip");
  expect(back.phase().si_value().allclose(phase, 2e-10, 2e-10),
         "batched true anomaly did not round trip");

  const auto matrix = simple_elem(
      axis.select(0, 0), ecc.select(0, 0), tilt.select(0, 0),
      node.select(0, 0), peri.select(0, 0), phase.select(0, 0));
  expect_shape(astro::from_elem(matrix, mu(7.0)).position().si_value(),
               {3, 4, 3}, "rank-three Cartesian output changed shape");

  const auto vector = simple_elem(
      axis.select(0, 0).select(0, 0), ecc.select(0, 0).select(0, 0),
      tilt.select(0, 0).select(0, 0), node.select(0, 0).select(0, 0),
      peri.select(0, 0).select(0, 0), phase.select(0, 0).select(0, 0));
  expect_shape(astro::from_elem(vector, mu(7.0)).position().si_value(), {4, 3},
               "rank-two Cartesian output changed shape");
}

void test_hyperbola_and_diagnostics() {
  const auto elem = simple_elem(
      pluss::series::make(-2.0), pluss::series::make(1.5),
      pluss::series::make(0.3), pluss::series::make(0.2),
      pluss::series::make(0.4), pluss::series::make(0.5));
  const auto state = astro::from_elem(elem, mu(5.0));
  const auto back = astro::to_elem(state, mu(5.0));
  expect(back.axis().si_value().allclose(elem.axis().si_value(), 1e-10, 1e-10),
         "hyperbolic axis did not round trip");
  expect(back.ecc().si_value().allclose(elem.ecc().si_value(), 1e-10, 1e-10),
         "hyperbolic eccentricity did not round trip");
  expect(close(astro::peri(elem).si_value().item<double>(), 1.0),
         "hyperbolic periapsis distance is incorrect");
  expect(astro::energy(state, mu(5.0)).si_value().item<double>() > 0.0,
         "hyperbolic specific energy must be positive");

  bool rejected_period = false;
  try {
    (void)astro::period(elem, mu(5.0));
  } catch (const std::domain_error &) {
    rejected_period = true;
  }
  expect(rejected_period, "hyperbolic orbit was assigned a period");

  const auto ellipse = simple_elem(
      pluss::series::make(4.0), pluss::series::make(0.25),
      pluss::series::make(0.0), pluss::series::make(0.0),
      pluss::series::make(0.0), pluss::series::make(0.0));
  expect(close(astro::peri(ellipse).si_value().item<double>(), 3.0),
         "elliptic periapsis distance is incorrect");
  expect(close(astro::apo(ellipse).si_value().item<double>(), 5.0),
         "elliptic apoapsis distance is incorrect");
  expect(close(astro::period(ellipse, mu(4.0)).si_value().item<double>(),
               8.0 * std::numbers::pi),
         "Kepler period is incorrect");
}

void test_graph_and_failures() {
  auto phase = pluss::series::make(0.35);
  phase.tracks_slope_(true);
  const auto elem = simple_elem(
      pluss::series::make(2.0), pluss::series::make(0.2),
      pluss::series::make(0.4), pluss::series::make(0.3),
      pluss::series::make(0.7), phase);
  astro::from_elem(elem, mu(3.0)).position().si_value().sum().reverse();
  expect(phase.has_sensitivity() &&
             std::isfinite(phase.sensitivity().item<double>()),
         "element conversion severed reverse history");

  bool parabolic = false;
  try {
    (void)simple_elem(pluss::series::make(1.0), pluss::series::make(1.0),
                      pluss::series::make(0.0), pluss::series::make(0.0),
                      pluss::series::make(0.0), pluss::series::make(0.0));
  } catch (const std::domain_error &) {
    parabolic = true;
  }
  expect(parabolic, "parabolic singularity was accepted");

  bool radial = false;
  try {
    const auto state = astro::make_state(
        common::SeriesQuantity<common::Length>::from_si(
            pluss::series::make({1.0, 0.0, 0.0})),
        common::SeriesQuantity<common::Speed>::from_si(
            pluss::series::make({1.0, 0.0, 0.0})));
    (void)astro::to_elem(state, mu(1.0));
  } catch (const std::domain_error &) {
    radial = true;
  }
  expect(radial, "radial trajectory was assigned a classical orbit plane");

  bool bad_mu = false;
  try {
    (void)astro::from_elem(elem, mu(0.0));
  } catch (const std::invalid_argument &) {
    bad_mu = true;
  }
  expect(bad_mu, "non-positive gravitational parameter was accepted");
}

void test_anomaly_solver() {
  const pluss::Series ecc = pluss::series::make(
      std::vector<double>{0.0, 0.2, 0.9, 0.999, 1.001, 1.2, 2.0, 5.0},
      {2, 2, 2});
  const pluss::Series mean = pluss::series::make(
      std::vector<double>{0.0, 0.1, -2.0, 40.0, -4.0, 0.25, 3.0, 20.0},
      {2, 2, 2});
  const auto solved = astro::solve_mean(ecc, mean);
  expect(solved.converged.all().item<bool>(),
         "mixed Kepler solve did not converge");
  expect(solved.residual.abs().max().item<double>() < 2e-12,
         "mixed Kepler solve retained a large residual");
  expect_shape(solved.anomaly, {2, 2, 2},
               "Kepler solve changed the batch shape");

  const pluss::Series phase = astro::to_true(ecc, solved.anomaly);
  const pluss::Series recovered = astro::to_mean(ecc, phase);
  const pluss::Series wrapped = mean.remainder(2.0 * std::numbers::pi);
  const pluss::Series ellipse_mean =
      wrapped.lt(0.0).where(wrapped.add(2.0 * std::numbers::pi), wrapped);
  const pluss::Series expected = ecc.lt(1.0).where(ellipse_mean, mean);
  expect(recovered.allclose(expected, 2e-11, 2e-11),
         "true/mean anomaly conversion did not round trip");

  const auto many_turns = astro::solve_mean(
      pluss::series::make(0.4),
      pluss::series::make(200.0 * std::numbers::pi + 0.3));
  expect(std::isfinite(astro::to_true(pluss::series::make(0.4),
                                     many_turns.anomaly)
                           .item<double>()),
         "many-revolution elliptic anomaly overflowed an unselected branch");
}

void test_two_body_advance() {
  const auto circle = simple_elem(
      pluss::series::make(4.0), pluss::series::make(0.0),
      pluss::series::make(0.0), pluss::series::make(0.0),
      pluss::series::make(0.0), pluss::series::make(0.0));
  const auto quarter = astro::advance(
      circle, common::ScalarQuantity<common::Time>::from_si(2.0 *
                                                            std::numbers::pi),
      mu(4.0));
  expect(close(quarter.elements.phase().si_value().item<double>(),
               std::numbers::pi / 2.0, 1e-11),
         "circular quarter-period advance is incorrect");
  expect(close(astro::mean_rate(circle, mu(4.0)).si_value().item<double>(),
               0.25),
         "mean motion is incorrect");
  const auto restored = astro::advance(
      quarter.elements,
      common::ScalarQuantity<common::Time>::from_si(-2.0 *
                                                    std::numbers::pi),
      mu(4.0));
  expect(close(restored.elements.phase().si_value().item<double>(), 0.0,
               1e-11),
         "negative elapsed time did not reverse circular propagation");

  const auto ellipse = simple_elem(
      pluss::series::make(3.0), pluss::series::make(0.6),
      pluss::series::make(0.4), pluss::series::make(0.2),
      pluss::series::make(0.7), pluss::series::make(1.3));
  const double orbit_period =
      astro::period(ellipse, mu(2.0)).si_value().item<double>();
  const auto full = astro::advance(
      ellipse, common::ScalarQuantity<common::Time>::from_si(orbit_period),
      mu(2.0));
  const auto before = astro::from_elem(ellipse, mu(2.0));
  const auto after = astro::from_elem(full.elements, mu(2.0));
  expect(after.position().si_value().allclose(before.position().si_value(),
                                              2e-11, 2e-11) &&
             after.velocity().si_value().allclose(before.velocity().si_value(),
                                                  2e-11, 2e-11),
         "one elliptic period did not return to the initial state");

  const auto hyperbola = simple_elem(
      pluss::series::make(-3.0), pluss::series::make(1.4),
      pluss::series::make(0.3), pluss::series::make(0.2),
      pluss::series::make(0.5), pluss::series::make(0.1));
  const auto escaped = astro::advance(
      hyperbola, common::ScalarQuantity<common::Time>::from_si(0.7), mu(2.0));
  const auto escaped_state = astro::from_elem(escaped.elements, mu(2.0));
  expect(escaped.solve.converged.item<bool>() &&
             escaped.solve.residual.abs().item<double>() < 1e-12 &&
             escaped_state.position().si_value().isfinite().all().item<bool>(),
         "hyperbolic propagation did not produce a finite converged state");

  const pluss::Series axis = pluss::series::full({2, 3, 4}, 5.0);
  const auto batch = simple_elem(
      axis, pluss::series::full_like(axis, 0.3),
      pluss::series::full_like(axis, 0.2),
      pluss::series::full_like(axis, 0.4),
      pluss::series::full_like(axis, 0.6),
      pluss::series::full_like(axis, 0.8));
  const auto elapsed = common::SeriesQuantity<common::Time>::from_si(
      pluss::series::make({0.0, 0.5})
          .to(axis.scalar_type())
          .reshape({2, 1, 1}));
  const auto moved = astro::advance(batch, elapsed, mu(3.0));
  expect_shape(moved.elements.phase().si_value(), {2, 3, 4},
               "batched elapsed time changed element shape");
  expect(moved.elements.phase().si_value().scalar_type() == axis.scalar_type(),
         "batched propagation promoted the orbit dtype");
  expect(moved.solve.converged.all().item<bool>(),
         "batched element propagation did not converge");
}

void test_advance_graph_and_convergence() {
  auto elapsed_value = pluss::series::make(0.2);
  elapsed_value.tracks_slope_(true);
  const auto elapsed =
      common::SeriesQuantity<common::Time>::from_si(elapsed_value);
  const auto circle = simple_elem(
      pluss::series::make(4.0), pluss::series::make(0.0),
      pluss::series::make(0.0), pluss::series::make(0.0),
      pluss::series::make(0.0), pluss::series::make(0.3));
  auto moved = astro::advance(circle, elapsed, mu(4.0));
  moved.elements.phase().si_value().reverse();
  expect(elapsed_value.has_sensitivity() &&
             close(elapsed_value.sensitivity().item<double>(), 0.25, 2e-8),
         "two-body advance lost the phase derivative with respect to time");

  auto eccentric_time = pluss::series::make(0.2);
  eccentric_time.tracks_slope_(true);
  const auto eccentric = simple_elem(
      pluss::series::make(4.0), pluss::series::make(0.5),
      pluss::series::make(0.0), pluss::series::make(0.0),
      pluss::series::make(0.0), pluss::series::make(0.4));
  auto eccentric_move = astro::advance(
      eccentric,
      common::SeriesQuantity<common::Time>::from_si(eccentric_time), mu(4.0));
  eccentric_move.elements.phase().si_value().reverse();
  const double final_phase =
      eccentric_move.elements.phase().si_value().item<double>();
  const double expected_rate =
      0.25 * std::pow(1.0 + 0.5 * std::cos(final_phase), 2.0) /
      std::pow(1.0 - 0.25, 1.5);
  expect(eccentric_time.has_sensitivity() &&
             close(eccentric_time.sensitivity().item<double>(), expected_rate,
                   2e-8),
         "eccentric propagation did not differentiate through Kepler solve");

  astro::KeplerOptions loose;
  loose.max_steps = 1;
  loose.tolerance = 1e-15;
  loose.check_every = 1;
  loose.strict = false;
  const auto incomplete = astro::solve_mean(
      pluss::series::make(0.999), pluss::series::make(0.02), loose);
  expect(!incomplete.converged.all().item<bool>(),
         "one-step diagnostic solve unexpectedly reported convergence");

  loose.strict = true;
  bool failed_closed = false;
  try {
    (void)astro::solve_mean(pluss::series::make(0.999),
                            pluss::series::make(0.02), loose);
  } catch (const std::runtime_error &) {
    failed_closed = true;
  }
  expect(failed_closed, "strict Kepler solve did not fail closed");
}

} // namespace

int main() {
  try {
    test_vallado_state();
    test_singular_conventions();
    test_high_rank_batches();
    test_hyperbola_and_diagnostics();
    test_graph_and_failures();
    test_anomaly_solver();
    test_two_body_advance();
    test_advance_graph_and_convergence();
    std::cout << "NatSci astro orbital checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}

