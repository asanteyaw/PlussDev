#include "pluss/codem/series_factory.h"
#include "pluss/ma/natsci/common.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::ma::natsci::common;
namespace solve = common::solve;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool close(double actual, double expected, double relative = 1e-9) {
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

void test_linear_interpolation() {
  auto raw_coordinate = pluss::series::make({0.0, 1.0, 2.0});
  raw_coordinate.tracks_slope_(true);
  const auto coordinate =
      common::SeriesQuantity<common::Time>::from_si(raw_coordinate);
  auto raw_values = pluss::series::make(
      std::vector<double>{0.0, 10.0, 20.0, 0.0, 1.0, 4.0}, {2, 3});
  raw_values.tracks_slope_(true);
  const auto values =
      common::SeriesQuantity<common::Length>::from_si(raw_values);
  auto raw_query = pluss::series::make({0.5, 1.5});
  raw_query.tracks_slope_(true);
  const auto query = common::SeriesQuantity<common::Time>::from_si(raw_query);

  const auto result = solve::interp(coordinate, values, query);
  expect_shape(result.si_value(), {2, 2},
               "batched interpolation changed output shape");
  expect(result.si_value().allclose(
             pluss::series::make({{5.0, 15.0}, {0.5, 2.5}}), 1e-12, 1e-12),
         "batched interpolation returned incorrect values");

  result.si_value().sum().reverse();
  expect(raw_coordinate.has_sensitivity() && raw_values.has_sensitivity() &&
             raw_query.has_sensitivity(),
         "interpolation severed reverse-mode history");

  const auto scalar_query =
      common::SeriesQuantity<common::Time>::from_si(pluss::series::make(0.25));
  const auto scalar_result = solve::interp(coordinate, values, scalar_query);
  expect_shape(scalar_result.si_value(), {2},
               "scalar interpolation retained a query axis");
  expect(scalar_result.si_value().allclose(pluss::series::make({2.5, 0.25}),
                                           1e-12, 1e-12),
         "scalar interpolation returned incorrect values");

  const auto outside = common::SeriesQuantity<common::Time>::from_si(
      pluss::series::make({-1.0, 3.0}));
  const auto clamped = solve::interp(coordinate, values, outside);
  expect(clamped.si_value().allclose(
             pluss::series::make({{0.0, 20.0}, {0.0, 4.0}}), 1e-12, 1e-12),
         "clamped interpolation did not retain endpoint values");

  const auto linear =
      solve::interp(coordinate, values, outside,
                    pluss::subsp::math::interp::EdgeMode::Linear);
  expect(linear.si_value().allclose(
             pluss::series::make({{-10.0, 30.0}, {-1.0, 7.0}}), 1e-12, 1e-12),
         "linear extrapolation used the wrong edge intervals");

  const auto strided_raw =
      pluss::series::make(
          std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, {2, 3, 2})
          .transpose(1, 2);
  expect(!strided_raw.is_contiguous(),
         "test precondition: interpolation values must be strided");
  const auto strided_values =
      common::SeriesQuantity<common::Length>::from_si(strided_raw);
  const auto strided_result = solve::interp(coordinate, strided_values, query);
  expect_shape(strided_result.si_value(), {2, 2, 2},
               "strided rank-3 interpolation changed batch axes");
  expect(strided_result.si_value().allclose(
             pluss::series::make(std::vector<double>{1, 3, 2, 4, 7, 9, 8, 10},
                                 {2, 2, 2}),
             1e-12, 1e-12),
         "strided rank-3 interpolation returned incorrect values");

  const auto rank4_values =
      common::SeriesQuantity<common::Length>::from_si(strided_raw.unsqueeze(1));
  const auto rank4_result = solve::interp(coordinate, rank4_values, query);
  expect_shape(rank4_result.si_value(), {2, 1, 2, 2},
               "rank-4 interpolation changed leading batch axes");

  bool rejected = false;
  try {
    (void)solve::interp(coordinate, values, outside,
                        pluss::subsp::math::interp::EdgeMode::Error);
  } catch (const std::out_of_range &) {
    rejected = true;
  }
  expect(rejected, "strict interpolation accepted an out-of-domain query");
}

void test_quadrature() {
  pluss::subsp::math::integ::QuadraOptions options;
  options.adaptive = false;
  options.nodes = 65;

  const auto result = solve::quad(
      [](const common::SeriesQuantity<common::Time> &time) {
        const auto factors = pluss::series::make({1.0, 2.0}).unsqueeze(-1);
        return common::SeriesQuantity<common::Speed>::from_si(
            factors.mul(time.si_value().mul(0.0).add(3.0)));
      },
      common::ScalarQuantity<common::Time>::from_si(0.0),
      common::ScalarQuantity<common::Time>::from_si(2.0), options);

  static_assert(
      std::same_as<typename decltype(result.value)::dimension, common::Length>);
  expect_shape(result.value.si_value(), {2},
               "batched quadrature changed leading dimensions");
  expect(result.value.si_value().allclose(pluss::series::make({6.0, 12.0}),
                                          1e-10, 1e-10),
         "dimension-aware quadrature returned incorrect values");
  expect(result.converged && result.evaluations > 0,
         "quadrature diagnostics were not forwarded");
}

void test_flow() {
  const auto initial = common::states(
      common::field("position", common::SeriesQuantity<common::Length>::from_si(
                                    pluss::series::make({0.0, 10.0}))),
      common::field("speed", common::SeriesQuantity<common::Speed>::from_si(
                                 pluss::series::make({1.0, -1.0}))));
  const auto parameters = common::params(common::param(
      "acceleration", common::SeriesQuantity<common::Acceleration>::from_si(
                          pluss::series::make({2.0, 1.0}))));

  pluss::subsp::math::ode::OdeOptions options;
  options.steps = 8;
  const auto result = solve::flow(
      [](common::ScalarQuantity<common::Time>, const common::StateVector &state,
         const common::ParameterSet &parameters) {
        return common::states(
            common::field("position", state.get<common::Speed>("speed")),
            common::field(
                "speed", parameters.get<common::Acceleration>("acceleration")));
      },
      initial, parameters, common::ScalarQuantity<common::Time>::from_si(0.0),
      common::ScalarQuantity<common::Time>::from_si(2.0), options);

  expect(result.converged && result.steps == 8 && result.evaluations == 32,
         "flow diagnostics did not match the delegated RK4 solve");
  expect(result.state.get<common::Length>("position")
             .si_value()
             .allclose(pluss::series::make({6.0, 10.0}), 1e-10, 1e-10),
         "flow position solution is incorrect");
  expect(result.state.get<common::Speed>("speed").si_value().allclose(
             pluss::series::make({5.0, 1.0}), 1e-10, 1e-10),
         "flow speed solution is incorrect");

  bool wrong_dimension = false;
  try {
    (void)solve::flow(
        [](common::ScalarQuantity<common::Time>,
           const common::StateVector &state) {
          return common::states(
              common::field("position", state.get<common::Length>("position")),
              common::field(
                  "speed",
                  common::SeriesQuantity<common::Acceleration>::from_si(
                      state.get<common::Speed>("speed").si_value().mul(0.0))));
        },
        initial, common::ScalarQuantity<common::Time>::from_si(0.0),
        common::ScalarQuantity<common::Time>::from_si(1.0), options);
  } catch (const std::invalid_argument &) {
    wrong_dimension = true;
  }
  expect(wrong_dimension,
         "flow accepted a derivative with the state's physical dimension");
}

void test_terminal_event() {
  const auto initial = common::states(
      common::field("position", common::SeriesQuantity<common::Length>::from_si(
                                    pluss::series::make(0.0))));
  pluss::subsp::math::ode::OdeOptions options;
  options.steps = 4;
  pluss::subsp::math::ode::OdeEventOptions event_options;
  event_options.direction = pluss::subsp::math::ode::OdeEventDirection::Rising;

  const auto derivative = [](common::ScalarQuantity<common::Time>,
                             const common::StateVector &state) {
    return common::states(common::field(
        "position",
        common::SeriesQuantity<common::Speed>::from_si(pluss::series::full_like(
            state.get<common::Length>("position").si_value(), 2.0))));
  };
  const auto event = [](common::ScalarQuantity<common::Time>,
                        const common::StateVector &state) {
    return common::SeriesQuantity<common::Dimensionless>::from_si(
        state.get<common::Length>("position").si_value().sub(5.0));
  };

  const auto hit =
      solve::flow_until(derivative, event, initial,
                        common::ScalarQuantity<common::Time>::from_si(0.0),
                        common::ScalarQuantity<common::Time>::from_si(4.0),
                        options, event_options);
  expect(hit.triggered && hit.converged, "terminal event was not detected");
  expect(close(hit.coordinate.si_value(), 2.5, 1e-9),
         "terminal event time was not refined");
  expect(
      close(hit.state.get<common::Length>("position").si_value().item<double>(),
            5.0, 1e-9),
      "terminal event state does not match the root");
  expect(hit.event_evaluations > 2,
         "terminal event did not report refinement evaluations");

  event_options.direction = pluss::subsp::math::ode::OdeEventDirection::Falling;
  const auto ignored =
      solve::flow_until(derivative, event, initial,
                        common::ScalarQuantity<common::Time>::from_si(0.0),
                        common::ScalarQuantity<common::Time>::from_si(4.0),
                        options, event_options);
  expect(!ignored.triggered && close(ignored.coordinate.si_value(), 4.0),
         "event direction filter accepted the wrong crossing");

  const auto parameters = common::params(
      common::param("speed", common::SeriesQuantity<common::Speed>::from_si(
                                 pluss::series::make(2.0))),
      common::param("threshold",
                    common::SeriesQuantity<common::Length>::from_si(
                        pluss::series::make(5.0))));
  event_options.direction = pluss::subsp::math::ode::OdeEventDirection::Rising;
  const auto parameterized = solve::flow_until(
      [](common::ScalarQuantity<common::Time>, const common::StateVector &state,
         const common::ParameterSet &parameters) {
        const pluss::Series &like =
            state.get<common::Length>("position").si_value();
        return common::states(common::field(
            "position",
            common::SeriesQuantity<common::Speed>::from_si(
                parameters.get<common::Speed>("speed").si_value().expand(
                    like.sizes()))));
      },
      [](common::ScalarQuantity<common::Time>, const common::StateVector &state,
         const common::ParameterSet &parameters) {
        return (state.get<common::Length>("position") -
                parameters.get<common::Length>("threshold")) /
               parameters.get<common::Length>("threshold");
      },
      initial, parameters, common::ScalarQuantity<common::Time>::from_si(0.0),
      common::ScalarQuantity<common::Time>::from_si(4.0), options,
      event_options);
  expect(parameterized.triggered &&
             close(parameterized.coordinate.si_value(), 2.5, 1e-9),
         "parameterized terminal event overload did not preserve parameters");
}

void test_dimensionless_fit() {
  const auto initial = common::params(
      common::param("length",
                    common::SeriesQuantity<common::Length>::from_si(
                        pluss::series::make(8.0)),
                    common::SeriesQuantity<common::Length>::from_si(
                        pluss::series::make(0.0)),
                    common::SeriesQuantity<common::Length>::from_si(
                        pluss::series::make(10.0))),
      common::param("time", common::SeriesQuantity<common::Time>::from_si(
                                pluss::series::make(1.0))));
  const auto scales = common::states(
      common::field("length", common::SeriesQuantity<common::Length>::from_si(
                                  pluss::series::make(10.0))),
      common::field("time", common::SeriesQuantity<common::Time>::from_si(
                                pluss::series::make(0.5))));

  solve::FitOptions options;
  options.solver = pluss::subsp::math::optim::presets::sequential_qp();
  options.solver.algorithm =
      pluss::subsp::math::optim::OptimaAlgorithm::WorkingSet;
  options.solver.working_set.max_iterations = 120;
  options.constraints.inequality = [](const common::ParameterSet &parameters) {
    return common::SeriesQuantity<common::Dimensionless>::from_si(
        parameters.get<common::Time>("time").si_value().sub(3.0).div(0.5));
  };

  const auto result = solve::fit(
      [](const common::ParameterSet &parameters) {
        const pluss::Series dx =
            parameters.get<common::Length>("length").si_value().sub(2.0).div(
                10.0);
        const pluss::Series dt =
            parameters.get<common::Time>("time").si_value().sub(4.0).div(0.5);
        return common::SeriesQuantity<common::Dimensionless>::from_si(
            dx.square().add(dt.square()).sum());
      },
      initial, scales, options);

  expect(result.solver.evaluation.status ==
             pluss::subsp::math::optim::EvalStatus::Ok,
         "dimensionless fit did not produce a valid evaluation");
  const double fitted_length =
      result.parameters.get<common::Length>("length").si_value().item<double>();
  expect(close(fitted_length, 2.0, 1e-5),
         "mixed-unit scaling changed the fitted length parameter: " +
             std::to_string(fitted_length) + ", normalized: " +
             std::to_string(
                 pluss::subsp::math::optim::flatten_values(result.solver.params)
                     .select(0, 0)
                     .item<double>()) +
             ", solver: " + result.solver.message);
  const double fitted_time =
      result.parameters.get<common::Time>("time").si_value().item<double>();
  expect(close(fitted_time, 3.0, 1e-5),
         "mixed-unit scaling changed the fitted time parameter: " +
             std::to_string(fitted_time) +
             ", solver: " + result.solver.message + ", objective: " +
             std::to_string(result.objective.si_value().item<double>()) +
             ", first-order: " +
             std::to_string(result.solver.output.first_order_optimality));

  bool bad_scale = false;
  try {
    const auto invalid_scales = common::states(
        common::field("length", common::SeriesQuantity<common::Length>::from_si(
                                    pluss::series::make(0.0))),
        common::field("time", common::SeriesQuantity<common::Time>::from_si(
                                  pluss::series::make(0.5))));
    (void)solve::fit(
        [](const common::ParameterSet &parameters) {
          return common::SeriesQuantity<common::Dimensionless>::from_si(
              parameters.get<common::Length>("length").si_value().square());
        },
        initial, invalid_scales);
  } catch (const std::invalid_argument &) {
    bad_scale = true;
  }
  expect(bad_scale, "fit accepted a zero physical scale");
}

} // namespace

int main() {
  try {
    test_linear_interpolation();
    test_quadrature();
    test_flow();
    test_terminal_event();
    test_dimensionless_fit();
    std::cout << "NatSci common solve adapters: all checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "NatSci common solve adapters failure: " << error.what()
              << '\n';
    return 1;
  }
}
