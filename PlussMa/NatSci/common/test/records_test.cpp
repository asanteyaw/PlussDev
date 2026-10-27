#include "pluss/codem/series_factory.h"
#include "pluss/ma/natsci/common.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::ma::natsci::common;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool close(double actual, double expected, double relative = 1e-10) {
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

void test_parameter_records() {
  const auto lengths = common::SeriesQuantity<common::Length>::from(
      pluss::series::make({1.0, 2.0, 3.0}), common::units::metre);
  const auto lower = common::SeriesQuantity<common::Length>::from(
      pluss::series::make({0.0, 0.0, 0.0}), common::units::metre);
  const auto upper = common::SeriesQuantity<common::Length>::from(
      pluss::series::make({4.0, 4.0, 4.0}), common::units::metre);
  const auto elapsed = common::SeriesQuantity<common::Time>::from(
      pluss::series::make({2.0, 4.0}), common::units::second);

  const auto model = common::params(
      common::param("length", lengths, lower, upper, "L", "calibration"),
      common::param("time", elapsed, "t", "clock"));

  expect(model.size() == 2 && model.contains("time"),
         "parameter metadata/index failed");
  expect(model.layout().total_numel() == 5,
         "parameter layout was not cached correctly");
  expect(close(model.get<common::Length>("length")
                   .in(common::units::metre)
                   .select(0, 2)
                   .item<double>(),
               3.0),
         "typed parameter retrieval failed");

  const auto rebound = model.repack(model.flat().mul(0.5));
  expect(close(rebound.get<common::Time>("time")
                   .in(common::units::second)
                   .select(0, 1)
                   .item<double>(),
               2.0),
         "parameter flatten/repack failed");

  bool wrong_dimension = false;
  try {
    (void)model.get<common::Mass>("length");
  } catch (const std::invalid_argument &) {
    wrong_dimension = true;
  }
  expect(wrong_dimension, "dimension-mismatched parameter access was accepted");

  bool bad_bound = false;
  try {
    (void)common::params(
        common::param("length", lengths,
                      common::SeriesQuantity<common::Length>::from_si(
                          pluss::series::make({2.0, 2.0, 2.0})),
                      upper));
  } catch (const std::invalid_argument &) {
    bad_bound = true;
  }
  expect(bad_bound, "parameter bound violation was accepted");

  bool integral_quantity = false;
  try {
    (void)common::SeriesQuantity<common::Length>::from(
        pluss::series::make({1, 2}, pluss::ScalarType::Int64),
        common::units::centimetre);
  } catch (const std::invalid_argument &) {
    integral_quantity = true;
  }
  expect(integral_quantity,
         "integral Series quantity allowed a truncating unit conversion");
}

void test_state_and_trajectory() {
  const auto base = pluss::series::make(
      std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, {2, 2, 3});
  const auto strided = base.transpose(1, 2);
  expect(!strided.is_contiguous(), "test precondition: state must be strided");

  const auto state = common::states(
      common::field("position",
                    common::SeriesQuantity<common::Length>::from_si(strided),
                    "x"),
      common::field(
          "speed",
          common::SeriesQuantity<common::Speed>::from_si(pluss::series::make(
              std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3})),
          "v"));
  expect(state.layout().total_numel() == 18,
         "state layout did not preserve all components");
  const auto state_roundtrip = state.repack(state.flat());
  expect(
      state_roundtrip.get<common::Length>("position").si_value().equal(strided),
      "strided state flatten/repack changed values");
  expect_shape(state_roundtrip.get<common::Length>("position").si_value(),
               {2, 3, 2}, "state shape was not restored");

  const auto time = common::SeriesQuantity<common::Time>::from_si(
      pluss::series::make(std::vector<double>{0, 1, 2, 10, 11, 12}, {2, 3}));
  const auto orbit = common::path(time, state);
  expect(orbit.steps() == 3 && orbit.ordered(),
         "batched trajectory contract failed");

  bool unordered = false;
  try {
    const auto bad_time = common::SeriesQuantity<common::Time>::from_si(
        pluss::series::make(std::vector<double>{0, 2, 1, 10, 11, 12}, {2, 3}));
    (void)common::path(bad_time, state);
  } catch (const std::invalid_argument &) {
    unordered = true;
  }
  expect(unordered, "unordered trajectory was accepted");

  const auto complex_state = common::states(common::field(
      "amplitude",
      common::SeriesQuantity<common::Dimensionless>::from_si(
          pluss::series::make(
              std::vector<std::complex<double>>{{1.0, 0.0}, {0.0, 1.0}},
              {2}))));
  const auto real_time = common::SeriesQuantity<common::Time>::from_si(
      pluss::series::make({0.0, 1.0}));
  expect(common::path(real_time, complex_state).steps() == 2,
         "real coordinate rejected a complex-valued state");
}

void test_affine_points() {
  const auto freezing =
      common::TemperaturePoint<>::from(0.0, common::units::celsius);
  const auto boiling =
      common::TemperaturePoint<>::from(212.0, common::units::fahrenheit);
  expect(close(freezing.in(common::units::fahrenheit), 32.0),
         "Celsius-to-Fahrenheit point conversion failed");
  expect(close((boiling - freezing).in(common::units::kelvin), 100.0),
         "temperature point difference failed");

  const auto points = common::TemperaturePoint<pluss::Series>::from(
      pluss::series::make({-40.0, 0.0, 100.0}), common::units::celsius);
  const auto shifted =
      points +
      common::SeriesQuantity<common::Temperature>::from(
          pluss::series::make({10.0, 10.0, 10.0}), common::units::kelvin);
  const auto shifted_c = shifted.in(common::units::celsius);
  expect(close(shifted_c.select(0, 0).item<double>(), -30.0) &&
             close(shifted_c.select(0, 2).item<double>(), 110.0),
         "batched affine point shift failed");
}

void test_series_uncertainty() {
  auto values = pluss::series::make({3.0, 4.0});
  values.tracks_slope_(true);
  const auto measured = common::Measurement<common::Length, pluss::Series>(
      common::SeriesQuantity<common::Length>::from_si(values),
      common::SeriesQuantity<common::Length>::from_si(
          pluss::series::make({0.3, 0.4})));
  const auto peer = common::Measurement<common::Length, pluss::Series>(
      common::SeriesQuantity<common::Length>::from_si(
          pluss::series::make({1.0, 2.0})),
      common::SeriesQuantity<common::Length>::from_si(
          pluss::series::make({0.4, 0.3})));
  const auto sum = measured + peer;
  expect(
      close(sum.standard_uncertainty().si_value().select(0, 0).item<double>(),
            0.5),
      "Series uncertainty propagation failed");
  sum.value().si_value().sum().reverse();
  expect(values.sensitivity().equal(pluss::series::make({1.0, 1.0})),
         "Series measurement severed reverse-mode history");

  const auto batch4 = common::Measurement<common::Time, pluss::Series>(
      common::SeriesQuantity<common::Time>::from_si(
          pluss::series::ones({2, 2, 2, 2}, pluss::ScalarType::Float64)),
      common::SeriesQuantity<common::Time>::from_si(
          pluss::series::full({2, 2, 2, 2}, 0.25, pluss::ScalarType::Float64)));
  expect_shape(batch4.upper().si_value(), {2, 2, 2, 2},
               "four-dimensional uncertainty lost batch shape");
}

void test_covariance() {
  using CovDim = common::DimensionPower<common::Length, 2>;
  const auto values = common::SeriesQuantity<common::Length>::from_si(
      pluss::series::make(std::vector<double>{1, 2, 3, 4}, {2, 2}));
  const auto covariance =
      common::SeriesQuantity<CovDim>::from_si(pluss::series::make(
          std::vector<double>{1, 0.5, 0.5, 4, 2, 0, 0, 3}, {2, 2, 2}));
  const common::CorrelatedMeasurement<common::Length> measured(
      values, covariance, common::CovarianceCheck::PositiveDefinite);
  const auto matrix =
      pluss::series::make(std::vector<double>{1, 2, 2, -1}, {2, 1, 2});
  const auto mapped = measured.map(matrix);

  expect_shape(mapped.values().si_value(), {2, 1},
               "batched covariance map value shape failed");
  expect_shape(mapped.covariance().si_value(), {2, 1, 1},
               "batched covariance map covariance shape failed");
  expect(close(mapped.values().si_value().select(0, 0).host<double>().front(), 5.0),
         "covariance map mean failed");
  expect(
      close(mapped.covariance().si_value().select(0, 0).host<double>().front(), 19.0),
      "covariance map propagation failed");

  bool asymmetric = false;
  try {
    (void)common::CorrelatedMeasurement<common::Length>(
        common::SeriesQuantity<common::Length>::from_si(
            pluss::series::make({1.0, 2.0})),
        common::SeriesQuantity<CovDim>::from_si(
            pluss::series::make({{1.0, 2.0}, {0.0, 1.0}})));
  } catch (const std::invalid_argument &) {
    asymmetric = true;
  }
  expect(asymmetric, "asymmetric covariance was accepted");

  const common::CorrelatedMeasurement<common::Length> singular(
      common::SeriesQuantity<common::Length>::from_si(
          pluss::series::make({1.0, 2.0})),
      common::SeriesQuantity<CovDim>::from_si(
          pluss::series::make({{0.0, 0.0}, {0.0, 1.0}})));
  expect(singular.check() == common::CovarianceCheck::PositiveSemidefinite,
         "singular positive-semidefinite covariance was rejected");

  bool indefinite = false;
  try {
    (void)common::CorrelatedMeasurement<common::Length>(
        common::SeriesQuantity<common::Length>::from_si(
            pluss::series::make({1.0, 2.0})),
        common::SeriesQuantity<CovDim>::from_si(
            pluss::series::make({{1.0, 2.0}, {2.0, 1.0}})));
  } catch (const std::invalid_argument &) {
    indefinite = true;
  }
  expect(indefinite, "indefinite covariance was accepted");

  const auto batch_values = common::SeriesQuantity<common::Length>::from_si(
      pluss::series::zeros({2, 3, 2}, pluss::ScalarType::Float64));
  const auto batch_cov = common::SeriesQuantity<CovDim>::from_si(
      pluss::series::eye(2, pluss::ScalarType::Float64)
          .reshape({1, 1, 2, 2})
          .expand({2, 3, 2, 2}));
  const common::CorrelatedMeasurement<common::Length> batch_measurement(
      batch_values, batch_cov);
  const auto batch_map =
      batch_measurement.map(pluss::series::make({{1.0, -1.0}}));
  expect_shape(batch_map.values().si_value(), {2, 3, 1},
               "four-dimensional covariance map lost batch axes");
  expect_shape(batch_map.covariance().si_value(), {2, 3, 1, 1},
               "four-dimensional covariance result shape failed");

  auto tracked_values = pluss::series::make({1.0, 2.0});
  auto tracked_cov = pluss::series::make({{2.0, 0.25}, {0.25, 1.0}});
  tracked_values.tracks_slope_(true);
  tracked_cov.tracks_slope_(true);
  const common::CorrelatedMeasurement<common::Length> tracked(
      common::SeriesQuantity<common::Length>::from_si(tracked_values),
      common::SeriesQuantity<CovDim>::from_si(tracked_cov));
  const auto tracked_map = tracked.map(pluss::series::make({{1.0, 2.0}}));
  tracked_map.values()
      .si_value()
      .sum()
      .add(tracked_map.covariance().si_value().sum())
      .reverse();
  expect(tracked_values.has_sensitivity() && tracked_cov.has_sensitivity(),
         "covariance map severed reverse-mode history");
}

} // namespace

int main() {
  try {
    test_parameter_records();
    test_state_and_trajectory();
    test_affine_points();
    test_series_uncertainty();
    test_covariance();
    std::cout << "NatSci common records: all checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "NatSci common records failure: " << error.what() << '\n';
    return 1;
  }
}
