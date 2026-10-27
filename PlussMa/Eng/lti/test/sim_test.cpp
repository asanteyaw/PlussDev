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
                  double tolerance = 2e-10) {
  const auto values = actual.to(pluss::ScalarType::Float64).host<double>();
  expect(values.size() == expected.size(), "simulation result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "simulation mismatch at lane " + std::to_string(lane));
  }
}

void expect_complex_close(const pluss::Series &actual,
                          const std::vector<std::complex<double>> &expected,
                          double tolerance = 2e-10) {
  const auto values =
      actual.to(pluss::ScalarType::Complex128).host<std::complex<double>>();
  expect(values.size() == expected.size(), "complex result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "complex simulation mismatch at lane " + std::to_string(lane));
  }
}

lti::StateSpace first_order(double sample_time = 0.0) {
  return lti::state(pluss::series::make(std::vector<double>{-1.0}, {1, 1}),
                    pluss::series::make(std::vector<double>{1.0}, {1, 1}),
                    pluss::series::make(std::vector<double>{1.0}, {1, 1}),
                    pluss::series::make(std::vector<double>{0.0}, {1, 1}),
                    sample_time);
}

template <class Fn> void expect_error(Fn &&fn, const std::string &message) {
  bool failed = false;
  try {
    fn();
  } catch (const pluss::Error &) {
    failed = true;
  }
  expect(failed, message);
}

} // namespace

int main() {
  const auto irregular_time = pluss::series::make({0.0, 0.2, 0.7, 1.5});
  const auto constant = pluss::series::ones({4, 1});
  const auto zoh = lti::sim(first_order(), constant, irregular_time,
                            lti::SimOptions{.hold = lti::Hold::Zero});
  expect(zoh.values.sizes() == std::vector<std::int64_t>{4, 1} &&
             zoh.states.sizes() == std::vector<std::int64_t>{4, 1},
         "continuous simulation shapes are wrong");
  expect_close(zoh.values, {0.0, 1.0 - std::exp(-0.2), 1.0 - std::exp(-0.7),
                            1.0 - std::exp(-1.5)});
  expect_close(zoh.states, zoh.values.flatten().pull<double>());

  const auto ramp = irregular_time.reshape({4, 1});
  const auto linear = lti::sim(first_order(), ramp, irregular_time,
                               lti::SimOptions{.hold = lti::Hold::Linear});
  expect_close(linear.values,
               {0.0, 0.2 - 1.0 + std::exp(-0.2), 0.7 - 1.0 + std::exp(-0.7),
                1.5 - 1.0 + std::exp(-1.5)});
  expect(
      !linear.values.allclose(lti::sim(first_order(), ramp, irregular_time,
                                       lti::SimOptions{.hold = lti::Hold::Zero})
                                  .values,
                              1e-12, 1e-12),
      "linear and zero-order holds became indistinguishable");

  const auto initial = pluss::series::make({2.0});
  const auto free = lti::sim(first_order(), pluss::series::zeros({4, 1}),
                             irregular_time, initial);
  expect_close(free.values, {2.0, 2.0 * std::exp(-0.2), 2.0 * std::exp(-0.7),
                             2.0 * std::exp(-1.5)});

  const auto singleton = lti::sim(
      first_order(), pluss::series::make(std::vector<double>{3.0}, {1, 1}),
      pluss::series::make({2.0}), pluss::series::make({4.0}));
  expect(singleton.values.sizes() == std::vector<std::int64_t>{1, 1} &&
             singleton.states.sizes() == std::vector<std::int64_t>{1, 1},
         "singleton simulation shapes are wrong");
  expect_close(singleton.values, {4.0});
  expect_close(singleton.states, {4.0});

  const auto float_system =
      lti::state(pluss::series::make(std::vector<float>{-1.0F}, {1, 1}),
                 pluss::series::make(std::vector<float>{1.0F}, {1, 1}),
                 pluss::series::make(std::vector<float>{1.0F}, {1, 1}),
                 pluss::series::make(std::vector<float>{0.0F}, {1, 1}));
  const auto float_run = lti::sim(
      float_system, pluss::series::ones({2, 1}, pluss::ScalarType::Float32),
      pluss::series::make({0.0F, 0.5F}),
      lti::SimOptions{.hold = lti::Hold::Zero});
  expect(float_run.values.scalar_type() == pluss::ScalarType::Float32 &&
             float_run.states.scalar_type() == pluss::ScalarType::Float32,
         "Float32 simulation promoted unexpectedly");
  expect_close(float_run.values, {0.0, 1.0 - std::exp(-0.5)}, 2e-6);

  const auto discrete =
      lti::state(pluss::series::make(std::vector<double>{0.5}, {1, 1}),
                 pluss::series::make(std::vector<double>{2.0}, {1, 1}),
                 pluss::series::make(std::vector<double>{3.0}, {1, 1}),
                 pluss::series::make(std::vector<double>{4.0}, {1, 1}), 0.25);
  const auto discrete_run =
      lti::sim(discrete,
               pluss::series::make(std::vector<double>{1.0, 2.0, -1.0}, {3, 1}),
               pluss::series::make({0.0, 0.25, 0.5}));
  expect_close(discrete_run.states, {0.0, 2.0, 5.0});
  expect_close(discrete_run.values, {4.0, 14.0, 11.0});

  const auto mimo = lti::state(pluss::series::make({{1.0, 0.0}, {0.0, 0.5}}),
                               pluss::series::make({{1.0, 0.0}, {0.0, 2.0}}),
                               pluss::series::make({{1.0, 1.0}, {1.0, -1.0}}),
                               pluss::series::zeros({2, 2}), 1.0);
  const auto mimo_run = lti::sim(
      mimo, pluss::series::make({{1.0, 2.0}, {3.0, -1.0}, {0.0, 0.0}}),
      pluss::series::make({0.0, 1.0, 2.0}), pluss::series::make({0.5, -0.5}));
  expect_close(mimo_run.states, {0.5, -0.5, 1.5, 3.75, 4.5, -0.125});
  expect_close(mimo_run.values, {0.0, 1.0, 5.25, -2.25, 4.375, 4.625});

  const auto batched_system = lti::state(
      pluss::series::make(std::vector<double>{-1.0, -2.0}, {2, 1, 1}),
      pluss::series::make(std::vector<double>{1.0}, {1, 1}),
      pluss::series::make(std::vector<double>{1.0}, {1, 1}),
      pluss::series::make(std::vector<double>{0.0}, {1, 1}));
  const auto batched_input = pluss::series::make(
      std::vector<double>{1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, {2, 3, 1});
  const auto batch_time = pluss::series::make({0.0, 0.5, 1.0});
  const auto batched =
      lti::sim(batched_system, batched_input, batch_time,
               pluss::series::make(std::vector<double>{0.0, 1.0}, {2, 1}),
               lti::SimOptions{.hold = lti::Hold::Zero});
  expect(batched.values.sizes() == std::vector<std::int64_t>{2, 3, 1},
         "batched simulation shape is wrong");
  expect_close(batched.values, {0.0, 1.0 - std::exp(-0.5), 1.0 - std::exp(-1.0),
                                1.0, 1.0, 1.0});

  const auto strided_input =
      pluss::series::make({{1.0, 1.0, 1.0, 1.0}}).transpose(0, 1);
  expect(!strided_input.is_contiguous(), "strided input became contiguous");
  expect_close(lti::sim(first_order(), strided_input, irregular_time).values,
               zoh.values.flatten().pull<double>());

  using Complex = std::complex<double>;
  const auto complex_system =
      lti::state(pluss::series::make(std::vector<Complex>{{0.0, 1.0}}, {1, 1}),
                 pluss::series::zeros({1, 1}, pluss::ScalarType::Complex128),
                 pluss::series::make(std::vector<Complex>{{1.0, 0.0}}, {1, 1}),
                 pluss::series::zeros({1, 1}, pluss::ScalarType::Complex128));
  const auto complex_run = lti::sim(
      complex_system, pluss::series::zeros({3, 1}),
      pluss::series::make({0.0, std::acos(-1.0) / 2.0, std::acos(-1.0)}),
      pluss::series::make(std::vector<Complex>{{1.0, 0.0}}));
  expect_complex_close(complex_run.values,
                       {{1.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}}, 2e-9);

  auto tracked_a = pluss::series::make(std::vector<double>{-1.0}, {1, 1});
  auto tracked_b = pluss::series::make(std::vector<double>{1.0}, {1, 1});
  auto tracked_c = pluss::series::make(std::vector<double>{1.0}, {1, 1});
  auto tracked_d = pluss::series::make(std::vector<double>{0.25}, {1, 1});
  auto tracked_u = pluss::series::make(std::vector<double>{1.0, 1.0}, {2, 1});
  auto tracked_x = pluss::series::make({0.2});
  tracked_a.tracks_slope_(true);
  tracked_b.tracks_slope_(true);
  tracked_c.tracks_slope_(true);
  tracked_d.tracks_slope_(true);
  tracked_u.tracks_slope_(true);
  tracked_x.tracks_slope_(true);
  const auto tracked_system =
      lti::state(tracked_a, tracked_b, tracked_c, tracked_d);
  lti::sim(tracked_system, tracked_u, pluss::series::make({0.0, 1.0}),
           tracked_x, lti::SimOptions{.hold = lti::Hold::Zero})
      .values.select(0, 1)
      .sum()
      .reverse();
  const double inv_e = std::exp(-1.0);
  expect_close(tracked_a.sensitivity(), {1.0 - 1.8 * inv_e}, 3e-8);
  expect_close(tracked_b.sensitivity(), {1.0 - inv_e}, 3e-8);
  expect_close(tracked_c.sensitivity(), {1.0 - 0.8 * inv_e}, 3e-8);
  expect_close(tracked_d.sensitivity(), {1.0}, 3e-8);
  expect_close(tracked_u.sensitivity(), {1.0 - inv_e, 0.25}, 3e-8);
  expect_close(tracked_x.sensitivity(), {inv_e}, 3e-8);

  expect_error(
      [&] {
        (void)lti::sim(first_order(), pluss::series::ones({4}), irregular_time);
      },
      "rank-one input was accepted");
  expect_error(
      [&] {
        (void)lti::sim(first_order(), pluss::series::ones({3, 1}),
                       irregular_time);
      },
      "mismatched input and time lengths were accepted");
  expect_error(
      [&] {
        (void)lti::sim(first_order(), pluss::series::ones({4, 2}),
                       irregular_time);
      },
      "wrong input width was accepted");
  expect_error(
      [&] {
        (void)lti::sim(first_order(), pluss::series::ones({3, 1}),
                       pluss::series::make({0.0, 0.5, 0.5}));
      },
      "non-increasing time was accepted");
  expect_error(
      [&] {
        (void)lti::sim(first_order(), pluss::series::ones({2, 1}),
                       pluss::series::make(
                           {0.0, std::numeric_limits<double>::infinity()}));
      },
      "non-finite time was accepted");
  expect_error(
      [&] {
        (void)lti::sim(discrete, pluss::series::ones({3, 1}),
                       pluss::series::make({0.0, 0.2, 0.5}));
      },
      "wrong discrete sample spacing was accepted");
  expect_error(
      [&] {
        (void)lti::sim(discrete, pluss::series::ones({3, 1}),
                       pluss::series::make({0.0, 0.25, 0.5}),
                       lti::SimOptions{.hold = lti::Hold::Linear});
      },
      "linear hold was accepted for a discrete system");
  expect_error(
      [&] {
        (void)lti::sim(first_order(), constant, irregular_time,
                       pluss::series::make({0.0, 1.0}));
      },
      "wrong initial-state width was accepted");
}
