#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng/lti.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
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
                  double tolerance = 1e-12) {
  const auto values = actual.to(pluss::ScalarType::Float64).to_vec<double>();
  expect(values.size() == expected.size(), "LTI result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "LTI value mismatch at lane " + std::to_string(lane));
  }
}

lti::StateSpace oscillator(double sample_time = 0.0) {
  return lti::state(pluss::series::make({{0.0, 1.0}, {-2.0, -3.0}}),
                    pluss::series::make(std::vector<double>{0.0, 1.0}, {2, 1}),
                    pluss::series::make({{1.0, 0.0}}),
                    pluss::series::make(std::vector<double>{0.0}, {1, 1}),
                    sample_time);
}

} // namespace

int main() {
  const auto continuous = oscillator();
  expect(continuous.is_cont() && !continuous.is_disc(),
         "continuous timebase contract failed");
  expect(continuous.states() == 2 && continuous.inputs() == 1 &&
             continuous.outputs() == 1,
         "state-space dimensions are wrong");
  expect_close(lti::ctrb(continuous), {0, 1, 1, -3});
  expect_close(lti::obsv(continuous), {1, 0, 0, 1});

  const auto discrete = oscillator(0.1);
  expect(discrete.is_disc() && discrete.sample_time() == 0.1,
         "discrete timebase contract failed");

  const auto batched = lti::state(
      pluss::series::make(std::vector<double>{0, 1, -2, -3, 0, 1, -4, -5},
                          {2, 2, 2}),
      pluss::series::make(std::vector<double>{0.0, 1.0}, {2, 1}),
      pluss::series::make({{1.0, 0.0}}),
      pluss::series::make(std::vector<double>{0.0}, {1, 1}));
  expect(batched.b().sizes()[0] == 2 && lti::ctrb(batched).sizes()[0] == 2,
         "state-space batch broadcasting failed");
  expect_close(lti::ctrb(batched), {0, 1, 1, -3, 0, 1, 1, -5});

  const auto strided_a =
      pluss::series::make({{0.0, -2.0}, {1.0, -3.0}}).transpose(0, 1);
  expect(!strided_a.is_contiguous(), "strided test input became contiguous");
  const auto strided = lti::state(
      strided_a, pluss::series::make(std::vector<double>{0.0, 1.0}, {2, 1}),
      pluss::series::make({{1.0, 0.0}}),
      pluss::series::make(std::vector<double>{0.0}, {1, 1}));
  expect_close(lti::ctrb(strided), {0, 1, 1, -3});

  const auto promoted =
      lti::state(pluss::series::make({{0.0F, 1.0F}, {-2.0F, -3.0F}}),
                 pluss::series::make(std::vector<double>{0.0, 1.0}, {2, 1}),
                 pluss::series::make({{1.0, 0.0}}),
                 pluss::series::make(std::vector<double>{0.0}, {1, 1}));
  expect(promoted.a().scalar_type() == pluss::ScalarType::Float64 &&
             promoted.b().scalar_type() == pluss::ScalarType::Float64,
         "state-space dtype promotion was inconsistent");

  using Complex = std::complex<double>;
  const auto complex_system = lti::state(
      pluss::series::make(
          std::vector<Complex>{{0.0, 1.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, -1.0}},
          {2, 2}),
      pluss::series::make(
          std::vector<Complex>{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}},
          {2, 2}),
      pluss::series::make(
          std::vector<Complex>{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}},
          {2, 2}),
      pluss::series::zeros({2, 2}, pluss::ScalarType::Complex128));
  expect(lti::ctrb(complex_system).scalar_type() ==
                 pluss::ScalarType::Complex128 &&
             lti::obsv(complex_system).sizes()[0] == 4,
         "complex state-space analysis failed");

  auto tracked_a = pluss::series::make({{0.0, 1.0}, {-2.0, -3.0}})
                       .to(pluss::ScalarType::Float64);
  tracked_a.tracks_slope_(true);
  const auto tracked = lti::state(
      tracked_a, pluss::series::make(std::vector<double>{0.0, 1.0}, {2, 1}),
      pluss::series::make({{1.0, 0.0}}),
      pluss::series::make(std::vector<double>{0.0}, {1, 1}));
  lti::ctrb(tracked).sum().reverse();
  expect_close(tracked_a.sensitivity(), {0.0, 1.0, 0.0, 1.0});

  bool bad_shape = false;
  try {
    (void)lti::state(pluss::series::make({{1.0, 0.0}, {0.0, 1.0}}),
                     pluss::series::make(std::vector<double>{1.0, 0.0}, {2, 1}),
                     pluss::series::make({{1.0, 0.0}}),
                     pluss::series::make({{0.0, 0.0}}));
  } catch (const pluss::Error &) {
    bad_shape = true;
  }
  expect(bad_shape, "incompatible feed-through shape was accepted");

  bool bad_time = false;
  try {
    (void)oscillator(-0.1);
  } catch (const pluss::Error &) {
    bad_time = true;
  }
  expect(bad_time, "negative sample time was accepted");

  bool infinite_time = false;
  try {
    (void)oscillator(std::numeric_limits<double>::infinity());
  } catch (const pluss::Error &) {
    infinite_time = true;
  }
  expect(infinite_time, "non-finite sample time was accepted");
}
