#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;

namespace {

using Complex = std::complex<double>;

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_close(const pluss::Series &actual,
                  const std::vector<double> &expected,
                  double tolerance = 2e-8) {
  const auto values = actual.to(pluss::ScalarType::Float64).host<double>();
  expect(values.size() == expected.size(), "real result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "real value mismatch at lane " + std::to_string(lane));
  }
}

void expect_complex_close(const pluss::Series &actual,
                          const std::vector<Complex> &expected,
                          double tolerance = 2e-8) {
  const auto values =
      actual.to(pluss::ScalarType::Complex128).host<Complex>();
  expect(values.size() == expected.size(), "complex result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "complex value mismatch at lane " + std::to_string(lane));
  }
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

pluss::Series scalar(double value) {
  return pluss::series::make(
      value, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
}

Complex integral(Complex z, double sample_time, control::PidRule rule) {
  switch (rule) {
  case control::PidRule::Forward:
    return sample_time / (z - 1.0);
  case control::PidRule::Backward:
    return sample_time * z / (z - 1.0);
  case control::PidRule::Trapezoid:
    return 0.5 * sample_time * (z + 1.0) / (z - 1.0);
  }
  throw std::runtime_error("invalid test rule");
}

} // namespace

int main() {
  const auto a = pluss::series::make({{0.0, 1.0}, {-2.0, -3.0}});
  const auto b = pluss::series::make(std::vector<double>{0.0, 1.0}, {2, 1});
  const auto poles =
      pluss::series::make(std::vector<Complex>{{-4.0, 0.0}, {-5.0, 0.0}}, {2});
  const auto siso = control::place(a, b, poles);
  expect_close(siso.gain, {18.0, 6.0});
  expect(siso.residual.item<double>() < 1e-10,
         "SISO assignment residual is too large");
  expect(siso.closed_loop.sizes() == std::vector<std::int64_t>{2, 2},
         "SISO closed-loop shape is wrong");

  const auto full =
      control::place(pluss::series::zeros({2, 2}, pluss::ScalarType::Float64),
                     pluss::series::eye(2, pluss::ScalarType::Float64),
                     pluss::series::make(
                         std::vector<Complex>{{-1.0, 2.0}, {-1.0, -2.0}}, {2}));
  expect_close(full.gain, {1.0, -2.0, 2.0, 1.0});
  expect(full.residual.item<double>() < 1e-10,
         "full-actuation complex-pair residual is too large");

  const auto repeated = control::place(
      pluss::series::zeros({2, 2}, pluss::ScalarType::Float64),
      pluss::series::eye(2, pluss::ScalarType::Float64),
      pluss::series::make(std::vector<Complex>{{-2.0, 0.0}, {-2.0, 0.0}}, {2}));
  expect_close(repeated.gain, {2.0, 0.0, 0.0, 2.0});

  const auto complex_plant = control::place(
      pluss::series::make(
          std::vector<Complex>{{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
          {2, 2}),
      pluss::series::make(
          std::vector<Complex>{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}},
          {2, 2}),
      pluss::series::make(std::vector<Complex>{{-1.0, 1.0}, {-2.0, 3.0}}, {2}));
  expect_complex_close(complex_plant.gain,
                       {{1.0, -1.0}, {0.0, 0.0}, {0.0, 0.0}, {2.0, -3.0}});

  const auto mimo_a = pluss::series::make(
      {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {-1.0, -2.0, -3.0}});
  const auto mimo_b = pluss::series::make(
      std::vector<double>{0.0, 0.0, 1.0, 0.0, 0.0, 1.0}, {3, 2});
  const auto mimo = control::place(
      mimo_a, mimo_b,
      pluss::series::make(
          std::vector<Complex>{{-1.0, 0.0}, {-2.0, 0.0}, {-4.0, 0.0}}, {3}));
  expect(mimo.gain.sizes() == std::vector<std::int64_t>{2, 3},
         "MIMO gain shape is wrong");
  expect(mimo.residual.item<double>() < 2e-8,
         "MIMO assignment residual is too large");
  expect(mimo.condition.item<double>() < 1e8,
         "MIMO basis is unexpectedly ill-conditioned");
  const auto mimo_trace = mimo.closed_loop.diagonal().sum();
  const auto mimo_trace_square =
      mimo.closed_loop.matmul(mimo.closed_loop).diagonal().sum();
  expect_close(mimo_trace, {-7.0});
  expect_close(mimo.closed_loop.linearalg.det(), {-8.0});
  expect_close(mimo_trace.mul(mimo_trace).sub(mimo_trace_square).mul(0.5),
               {14.0});

  const auto mimo_pair = control::place(
      mimo_a, mimo_b,
      pluss::series::make(
          std::vector<Complex>{{-1.0, 0.0}, {-2.0, 1.0}, {-2.0, -1.0}}, {3}));
  expect(mimo_pair.residual.item<double>() < 2e-8,
         "rank-deficient MIMO conjugate-pair assignment failed");
  expect_close(mimo_pair.closed_loop.diagonal().sum(), {-5.0});
  expect_close(mimo_pair.closed_loop.linearalg.det(), {-5.0});

  const auto mimo_batch = control::place(
      pluss::series::pile({mimo_a, mimo_a}, 0),
      pluss::series::pile({mimo_b, mimo_b}, 0),
      pluss::series::make(
          std::vector<Complex>{{-1.0, 0.0}, {-2.0, 0.0}, {-4.0, 0.0}}, {3}));
  expect(mimo_batch.gain.sizes() == std::vector<std::int64_t>{2, 2, 3} &&
             mimo_batch.residual.sizes() == std::vector<std::int64_t>{2},
         "batched MIMO pole assignment lost leading axes");
  expect(mimo_batch.residual.amax().item<double>() < 2e-8,
         "batched MIMO pole assignment residual is too large");

  const auto batch_a = pluss::series::make(
      std::vector<double>{0.0, 1.0, -2.0, -3.0, 0.0, 1.0, -6.0, -5.0},
      {2, 2, 2});
  const auto batch_b =
      pluss::series::make(std::vector<double>{0.0, 1.0, 0.0, 1.0}, {2, 2, 1});
  const auto batch = control::place(batch_a, batch_b, poles);
  expect(batch.gain.sizes() == std::vector<std::int64_t>{2, 1, 2},
         "batched pole-placement gain shape is wrong");
  expect_close(batch.gain, {18.0, 6.0, 14.0, 4.0});
  expect(batch.residual.sizes() == std::vector<std::int64_t>{2},
         "batched assignment diagnostics lost the batch axis");

  const auto float_place = control::place(
      a.to(pluss::ScalarType::Float32), b.to(pluss::ScalarType::Float32),
      poles.to(pluss::ScalarType::Complex64));
  expect(float_place.gain.scalar_type() == pluss::ScalarType::Float32,
         "Float32 pole assignment promoted unexpectedly");
  expect_close(float_place.gain, {18.0, 6.0}, 3e-4);

  const auto strided_place = control::place(
      pluss::series::make({{0.0, -2.0}, {1.0, -3.0}}).transpose(0, 1), b,
      poles);
  expect(!pluss::series::make({{0.0, -2.0}, {1.0, -3.0}})
              .transpose(0, 1)
              .is_contiguous(),
         "pole-placement strided fixture became contiguous");
  expect_close(strided_place.gain, {18.0, 6.0});

  const auto plant =
      lti::state(a, b, pluss::series::make({{1.0, 0.0}}),
                 pluss::series::zeros({1, 1}, pluss::ScalarType::Float64));
  expect_close(control::place(plant, poles).gain, {18.0, 6.0});

  const auto continuous =
      control::pid(scalar(2.0), scalar(3.0), scalar(4.0), scalar(0.5));
  const auto omega = pluss::series::make(std::vector<double>{0.5, 1.0}, {2});
  std::vector<Complex> expected_continuous;
  for (const double frequency : {0.5, 1.0}) {
    const Complex s{0.0, frequency};
    expected_continuous.push_back(2.0 + 3.0 / s + 4.0 * s / (0.5 * s + 1.0));
  }
  expect_complex_close(control::freqresp(continuous, omega),
                       expected_continuous);

  for (const control::PidRule rule :
       {control::PidRule::Forward, control::PidRule::Backward,
        control::PidRule::Trapezoid}) {
    const auto discrete = control::pid(scalar(1.5), scalar(0.7), scalar(0.2),
                                       scalar(0.4), 0.1, rule, rule);
    std::vector<Complex> expected;
    for (const double frequency : {0.5, 1.0}) {
      const Complex z = std::exp(Complex{0.0, 0.1 * frequency});
      const Complex op = integral(z, 0.1, rule);
      expected.push_back(1.5 + 0.7 * op + 0.2 / (0.4 + op));
    }
    expect_complex_close(control::freqresp(discrete, omega), expected, 5e-8);
  }

  const auto discrete_p =
      control::pid(scalar(2.0), scalar(0.0), scalar(0.0), scalar(0.0), 0.1);
  const auto dc = control::freqresp(
      discrete_p, pluss::series::make(std::vector<double>{0.0, 1.0}, {2}));
  expect_complex_close(dc, {{2.0, 0.0}, {2.0, 0.0}});
  expect(dc.real().isfinite().all().item<bool>() &&
             dc.imag().isfinite().all().item<bool>(),
         "disabled discrete integral contaminated the zero-frequency lane");

  const auto pid_batch =
      control::pid(pluss::series::make(std::vector<float>{1.0F, 2.0F}, {2}),
                   pluss::series::make(std::vector<float>{0.0F, 0.5F}, {2}),
                   pluss::series::make(std::vector<float>{0.0F, 0.0F}, {2}),
                   pluss::series::make(std::vector<float>{0.0F, 0.0F}, {2}));
  const auto pid_batch_response = control::freqresp(
      pid_batch, pluss::series::make(std::vector<float>{0.5F, 1.0F}, {2}));
  expect(pid_batch_response.sizes() == std::vector<std::int64_t>{2, 2},
         "PID parameter batches did not broadcast over frequency");
  expect(pid_batch_response.scalar_type() == pluss::ScalarType::Complex64,
         "Float32 PID response promoted unexpectedly");

  auto tracked_kp = scalar(2.0);
  tracked_kp.tracks_slope_(true);
  control::freqresp(
      control::pid(tracked_kp, scalar(0.0), scalar(0.0), scalar(0.0)), omega)
      .real()
      .sum()
      .reverse();
  expect_close(tracked_kp.sensitivity(), {2.0});

  expect_error(
      [&] {
        (void)control::place(
            a, pluss::series::zeros({2, 1}, pluss::ScalarType::Float64), poles);
      },
      "uncontrollable system was accepted");
  expect_error(
      [&] {
        (void)control::place(
            a, b,
            pluss::series::make(std::vector<Complex>{{-1.0, 2.0}, {-3.0, 0.0}},
                                {2}));
      },
      "unpaired complex pole was accepted for a real system");
  expect_error(
      [&] {
        const auto repeated = pluss::series::make(
            std::vector<Complex>{{-2.0, 0.0}, {-2.0, 0.0}}, {2});
        (void)control::place(a, b, repeated);
      },
      "SISO repeated pole beyond the input rank was accepted");
  expect_error(
      [&] {
        (void)control::pid(scalar(1.0), scalar(0.0), scalar(1.0), scalar(0.0),
                           0.1, control::PidRule::Forward,
                           control::PidRule::Trapezoid);
      },
      "unstable unfiltered trapezoidal derivative was accepted");
  expect_error(
      [&] {
        (void)control::pid(scalar(1.0), scalar(0.0), scalar(1.0), scalar(0.04),
                           0.1, control::PidRule::Forward,
                           control::PidRule::Forward);
      },
      "unstable forward derivative discretization was accepted");
  expect_error(
      [&] {
        (void)control::pid(scalar(1.0), scalar(0.0), scalar(0.0), scalar(-0.1));
      },
      "negative derivative filter was accepted");
  expect_error(
      [&] {
        (void)control::pid(scalar(1.0), scalar(0.0), scalar(0.0), scalar(0.0),
                           0.0, static_cast<control::PidRule>(99),
                           control::PidRule::Forward);
      },
      "invalid PID rule was accepted");
}
