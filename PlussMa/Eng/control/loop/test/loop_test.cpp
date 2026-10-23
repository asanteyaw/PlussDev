#include "pluss/codem/series_factory.h"
#include "pluss/subsp/eng/control.h"
#include "pluss/subsp/eng/lti.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_close(const pluss::Series &actual,
                  const std::vector<double> &expected,
                  double tolerance = 1e-8) {
  const auto values = actual.to(pluss::ScalarType::Float64).to_vec<double>();
  expect(values.size() == expected.size(), "loop result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "loop value mismatch at lane " + std::to_string(lane));
  }
}

void expect_complex_close(const pluss::Series &actual,
                          const std::vector<std::complex<double>> &expected,
                          double tolerance = 1e-8) {
  const auto values =
      actual.to(pluss::ScalarType::Complex128).to_vec<std::complex<double>>();
  expect(values.size() == expected.size(), "loop complex size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "loop complex mismatch at lane " + std::to_string(lane));
  }
}

template <class Fn>
void expect_error(Fn &&function, const std::string &message) {
  bool failed = false;
  try {
    function();
  } catch (const std::exception &) {
    failed = true;
  }
  expect(failed, message);
}

pluss::Series matrix1(double value,
                      pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  return pluss::series::full({1, 1}, value, pluss::Config{}.with_dtype(dtype));
}

lti::StateSpace
first_order(double gain = 1.0, double sample_time = 0.0,
            pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  return lti::state(matrix1(-1.0, dtype), matrix1(1.0, dtype),
                    matrix1(gain, dtype), matrix1(0.0, dtype), sample_time);
}

lti::PoleZero third_order_loop() {
  const auto config = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  return lti::zpk(pluss::series::empty({0}, config),
                  pluss::series::make({0.0, -1.0, -2.0}),
                  pluss::series::make(1.0));
}

} // namespace

int main() {
  const pluss::Series omega = pluss::series::make({0.0, 1.0});
  const auto scalar_loop = control::loop(first_order(), omega);
  expect_complex_close(scalar_loop.response, {{1.0, 0.0}, {0.5, -0.5}});
  expect_complex_close(scalar_loop.sens, {{0.5, 0.0}, {0.6, 0.2}});
  expect_complex_close(scalar_loop.comp, {{0.5, 0.0}, {0.4, -0.2}});
  expect_complex_close(scalar_loop.sens.add(scalar_loop.comp),
                       {{1.0, 0.0}, {1.0, 0.0}});

  const auto mimo =
      lti::state(pluss::series::make({{-1.0, 0.0}, {0.0, -2.0}}),
                 pluss::series::eye(2, pluss::ScalarType::Float64),
                 pluss::series::make({{1.0, 0.25}, {-0.5, 1.0}}),
                 pluss::series::zeros({2, 2}, pluss::ScalarType::Float64));
  const auto mimo_loop = control::loop(mimo, pluss::series::make({0.25, 2.0}));
  expect(mimo_loop.sens.sizes() == std::vector<std::int64_t>{2, 2, 2},
         "MIMO sensitivity shape is wrong");
  expect_complex_close(mimo_loop.sens.add(mimo_loop.comp),
                       {{1.0, 0.0},
                        {1.0, 0.0},
                        {0.0, 0.0},
                        {0.0, 0.0},
                        {0.0, 0.0},
                        {0.0, 0.0},
                        {1.0, 0.0},
                        {1.0, 0.0}},
                       2e-7);

  const pluss::Series dense = pluss::series::logspace(
      -3.0, 3.0, 20001, 10.0,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const lti::PoleZero plant = third_order_loop();
  const auto pole_loop = control::loop(plant, pluss::series::make({0.5, 1.0}));
  expect_complex_close(pole_loop.sens.add(pole_loop.comp),
                       {{1.0, 0.0}, {1.0, 0.0}});
  const auto margins = control::margin(plant, dense);
  expect_close(margins.gain, {6.0}, 3e-5);
  expect_close(margins.gain_freq, {std::sqrt(2.0)}, 3e-5);
  expect_close(margins.phase, {53.41078617769919}, 3e-5);
  expect_close(margins.phase_freq, {0.44574795963189456}, 3e-5);
  expect(margins.stability.item<double>() > 0.0 &&
             margins.stability.item<double>() < 1.0,
         "stability margin is outside its expected range");

  const pluss::Series response = lti::freqresp(plant, dense);
  const pluss::Series response_batch =
      response.unsqueeze(0).unite({response.mul(2.0).unsqueeze(0)}, 0, true);
  const auto batch_margins = control::margin(response_batch, dense);
  expect(batch_margins.gain.sizes() == std::vector<std::int64_t>{2},
         "batch margin shape is wrong");
  expect_close(batch_margins.gain, {6.0, 3.0}, 3e-5);
  expect_close(batch_margins.gain_freq, {std::sqrt(2.0), std::sqrt(2.0)}, 3e-5);

  const auto absent =
      control::margin(first_order(0.5),
                      pluss::series::logspace(-3.0, 3.0, 2001, 10.0,
                                              pluss::Config{}.with_dtype(
                                                  pluss::ScalarType::Float64)));
  expect(std::isinf(absent.gain.item<double>()) &&
             std::isinf(absent.phase.item<double>()),
         "absent crossovers did not produce infinite margins");
  expect(std::isnan(absent.gain_freq.item<double>()) &&
             std::isnan(absent.phase_freq.item<double>()),
         "absent crossovers did not produce NaN frequencies");

  const pluss::Series float_grid = pluss::series::logspace(
      -2.0, 1.0, 257, 10.0,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32));
  const auto float_margin = control::margin(
      first_order(2.0, 0.0, pluss::ScalarType::Float32), float_grid);
  expect(float_margin.gain.scalar_type() == pluss::ScalarType::Float32 &&
             float_margin.phase.scalar_type() == pluss::ScalarType::Float32,
         "margin promoted a Float32 model");

  const pluss::Series padded =
      pluss::series::make({0.01, 99.0, 0.1, 99.0, 1.0, 99.0, 10.0, 99.0});
  const pluss::Series strided = padded.slice(0, 0, 8, 2);
  const auto strided_margin = control::margin(
      lti::freqresp(first_order(2.0), strided).select(-3, 0).select(-2, 0),
      strided);
  expect(strided_margin.phase.defined() &&
             strided_margin.stability.isfinite().item<bool>(),
         "strided frequency data failed");

  pluss::Series tracked_a = matrix1(-1.0);
  tracked_a.tracks_slope_(true);
  const auto tracked_system =
      lti::state(tracked_a, matrix1(1.0), matrix1(1.0), matrix1(0.0));
  control::loop(tracked_system, pluss::series::make({0.5, 1.0}))
      .sens.abs()
      .sum()
      .reverse();
  expect(tracked_a.sensitivity().defined() &&
             tracked_a.sensitivity().isfinite().all().item<bool>(),
         "loop sensitivity lost the reverse graph");

  expect_error(
      [&] { (void)control::margin(mimo, pluss::series::make({0.1, 1.0})); },
      "classical MIMO margins were accepted");
  expect_error(
      [&] {
        (void)control::margin(first_order(1.0, 0.1),
                              pluss::series::make({0.0, 40.0}));
      },
      "a frequency above discrete Nyquist was accepted");
  expect_error(
      [&] {
        (void)control::margin(pluss::series::make({1.0, 0.5}),
                              pluss::series::make({0.1, 1.0}));
      },
      "a real-valued frequency response was accepted");
  expect_error(
      [&] {
        (void)control::margin(first_order(), pluss::series::make({1.0, 0.5}));
      },
      "a decreasing frequency grid was accepted");

  return 0;
}
