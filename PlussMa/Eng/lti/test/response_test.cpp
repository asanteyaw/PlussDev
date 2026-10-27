#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng/lti.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <stdexcept>
#include <string>
#include <vector>

namespace lti = pluss::subsp::eng::lti;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_close(const pluss::Series &actual,
                  const std::vector<double> &expected,
                  double tolerance = 1e-10) {
  const auto values = actual.to(pluss::ScalarType::Float64).host<double>();
  expect(values.size() == expected.size(), "LTI result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "LTI value mismatch at lane " + std::to_string(lane));
  }
}

void expect_complex_close(const pluss::Series &actual,
                          const std::vector<std::complex<double>> &expected,
                          double tolerance = 1e-10) {
  const auto values =
      actual.to(pluss::ScalarType::Complex128).host<std::complex<double>>();
  expect(values.size() == expected.size(), "LTI complex size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "LTI complex mismatch at lane " + std::to_string(lane));
  }
}

lti::StateSpace oscillator(double sample_time = 0.0) {
  return lti::state(pluss::series::make({{0.0, 1.0}, {-2.0, -3.0}}),
                    pluss::series::make(std::vector<double>{0.0, 1.0}, {2, 1}),
                    pluss::series::make({{1.0, 0.0}}),
                    pluss::series::make(std::vector<double>{0.0}, {1, 1}),
                    sample_time);
}

pluss::Series matrix1(double value) {
  return pluss::series::make(std::vector<double>{value}, {1, 1});
}

} // namespace

int main() {
  const auto omega = pluss::series::make({0.0, 1.0});
  const auto response = lti::freqresp(oscillator(), omega);
  expect(response.dim() == 3 && response.size(0) == 1 &&
             response.size(1) == 1 && response.size(2) == 2,
         "continuous response shape is wrong");
  expect_complex_close(response, {{0.5, 0.0}, {0.1, -0.3}});

  const double pi = std::acos(-1.0);
  const auto discrete =
      lti::state(matrix1(0.5), matrix1(1.0), matrix1(1.0), matrix1(0.0), 0.1);
  expect_complex_close(
      lti::freqresp(discrete, pluss::series::make({0.0, pi / 0.1})),
      {{2, 0}, {-2.0 / 3.0, 0}});

  const auto batch = lti::state(
      pluss::series::make(std::vector<double>{-1.0, -2.0}, {2, 1, 1}),
      matrix1(1.0), matrix1(1.0), matrix1(0.0));
  const auto batch_response =
      lti::freqresp(batch, pluss::series::make({0.0, 1.0}));
  expect(batch_response.dim() == 4 && batch_response.size(0) == 2 &&
             batch_response.size(-1) == 2,
         "batched response shape is wrong");
  expect_complex_close(batch_response,
                       {{1, 0}, {0.5, -0.5}, {0.5, 0}, {0.4, -0.2}});

  auto tracked_a = matrix1(-2.0);
  tracked_a.tracks_slope_(true);
  const auto tracked =
      lti::state(tracked_a, matrix1(1.0), matrix1(1.0), matrix1(0.0));
  lti::freqresp(tracked, pluss::series::make({0.0})).real().sum().reverse();
  expect_close(tracked_a.sensitivity(), {0.25}, 1e-9);

  bool integer_grid = false;
  try {
    (void)lti::freqresp(oscillator(), pluss::series::make({0, 1}));
  } catch (const pluss::Error &) {
    integer_grid = true;
  }
  expect(integer_grid, "integer frequency grid was accepted");

  bool scalar_grid = false;
  try {
    (void)lti::freqresp(oscillator(), pluss::series::make(0.0));
  } catch (const pluss::Error &) {
    scalar_grid = true;
  }
  expect(scalar_grid, "scalar frequency grid was accepted");
}
