#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <algorithm>
#include <cmath>
#include <complex>
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
                  double tolerance = 2e-9) {
  const auto values = actual.to(pluss::ScalarType::Float64).to_vec<double>();
  expect(values.size() == expected.size(), "control result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "control value mismatch at lane " + std::to_string(lane));
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

lti::StateSpace scalar_system(double a, double b, double c,
                              double sample_time = 0.0) {
  return lti::state(pluss::series::make(std::vector<double>{a}, {1, 1}),
                    pluss::series::make(std::vector<double>{b}, {1, 1}),
                    pluss::series::make(std::vector<double>{c}, {1, 1}),
                    pluss::series::zeros({1, 1}), sample_time);
}

} // namespace

int main() {
  const auto scalar = [](double value) {
    return pluss::series::make(std::vector<double>{value}, {1, 1});
  };

  expect_close(scalar(2.0).linearalg.sylv(scalar(3.0), scalar(10.0)), {2.0});
  const auto sylv_a = pluss::series::make({{1.0, 2.0}, {0.0, 3.0}});
  const auto sylv_b = pluss::series::make({{-2.0, 0.0}, {1.0, 4.0}});
  expect_close(sylv_a.linearalg.sylv(
                   sylv_b, pluss::series::make({{7.0, 18.0}, {7.0, 28.0}})),
               {1.0, 2.0, 3.0, 4.0});
  const auto sylv_batch =
      pluss::series::make(std::vector<double>{2.0, 4.0}, {2, 1, 1})
          .linearalg.sylv(
              scalar(1.0),
              pluss::series::make(std::vector<double>{6.0, 10.0}, {2, 1, 1}));
  expect(sylv_batch.sizes() == std::vector<std::int64_t>{2, 1, 1},
         "Sylvester batch shape is wrong");
  expect_close(sylv_batch, {2.0, 2.0});
  expect_close(scalar(-2.0).linearalg.lyap(scalar(4.0)), {1.0});
  expect_close(scalar(0.5).linearalg.dlyap(scalar(3.0)), {4.0});

  const auto a = pluss::series::make({{-1.0, 2.0}, {0.0, -3.0}});
  const auto q = pluss::series::make({{2.0, 0.5}, {0.5, 1.0}});
  const auto x = a.linearalg.lyap(q);
  const auto residual = a.matmul(x).add(x.matmul(a.transpose(-2, -1))).add(q);
  expect(residual.abs().amax().item<double>() < 2e-9,
         "coupled Lyapunov residual is too large");
  expect(x.allclose(x.transpose(-2, -1), 2e-10, 2e-10),
         "Lyapunov result lost symmetry");

  const auto batched_a =
      pluss::series::make(std::vector<double>{-1.0, -2.0}, {2, 1, 1});
  const auto batched_x = batched_a.linearalg.lyap(scalar(2.0));
  expect(batched_x.sizes() == std::vector<std::int64_t>{2, 1, 1},
         "Lyapunov batch broadcasting failed");
  expect_close(batched_x, {1.0, 0.5});
  const auto strided_a =
      pluss::series::make({{-1.0, 0.0}, {2.0, -3.0}}).transpose(0, 1);
  expect(!strided_a.is_contiguous(),
         "Lyapunov strided input became contiguous");
  const auto strided_x = strided_a.linearalg.lyap(q);
  expect(strided_a.matmul(strided_x)
                 .add(strided_x.matmul(strided_a.transpose(-2, -1)))
                 .add(q)
                 .abs()
                 .amax()
                 .item<double>() < 2e-9,
         "strided Lyapunov residual is too large");

  using Complex = std::complex<double>;
  const auto complex_a =
      pluss::series::make(std::vector<Complex>{{-1.0, 2.0}}, {1, 1});
  const auto complex_q =
      pluss::series::make(std::vector<Complex>{{2.0, 0.0}}, {1, 1});
  const auto complex_x = complex_a.linearalg.lyap(complex_q);
  expect(std::abs(complex_x.to(pluss::ScalarType::Complex128)
                      .item<std::complex<double>>() -
                  Complex{1.0, 0.0}) < 2e-9,
         "complex Lyapunov adjoint convention is wrong");
  const auto complex_care = complex_a.linearalg.care(
      pluss::series::make(std::vector<Complex>{{1.0, 0.0}}, {1, 1}),
      pluss::series::make(std::vector<Complex>{{1.0, 0.0}}, {1, 1}));
  expect(std::abs(complex_care.item<std::complex<double>>() -
                  Complex{std::sqrt(2.0) - 1.0, 0.0}) < 2e-8,
         "complex CARE used transpose instead of adjoint");

  const auto batched_care = batched_a.linearalg.care(scalar(1.0), scalar(1.0));
  expect_close(batched_care, {-1.0 + std::sqrt(2.0), -2.0 + std::sqrt(5.0)},
               2e-8);

  auto tracked_q = scalar(3.0);
  tracked_q.tracks_slope_(true);
  scalar(0.5).linearalg.dlyap(tracked_q).sum().reverse();
  expect_close(tracked_q.sensitivity(), {4.0 / 3.0}, 2e-8);

  const auto continuous = scalar_system(-2.0, 1.0, 1.0);
  expect_close(lti::gram(continuous, lti::Gram::Reach), {0.25});
  expect_close(lti::gram(continuous, lti::Gram::Observe), {0.25});
  const auto unequal = scalar_system(-2.0, 3.0, 4.0);
  expect_close(lti::gram(unequal, lti::Gram::Reach), {2.25});
  expect_close(lti::gram(unequal, lti::Gram::Observe), {4.0});

  const auto q4 = scalar(4.0);
  const auto r1 = scalar(1.0);
  const auto continuous_lqr = control::lqr(continuous, q4, r1);
  const double care_value = -2.0 + std::sqrt(8.0);
  expect_close(continuous_lqr.value, {care_value});
  expect_close(continuous_lqr.gain, {care_value});
  expect_close(continuous_lqr.closed_loop, {-2.0 - care_value});

  auto tracked_cost = scalar(4.0);
  tracked_cost.tracks_slope_(true);
  control::lqr(continuous, tracked_cost, r1).value.sum().reverse();
  expect_close(tracked_cost.sensitivity(), {1.0 / (2.0 * (care_value + 2.0))},
               2e-7);

  const auto discrete = scalar_system(0.5, 1.0, 1.0, 1.0);
  const auto discrete_lqr = control::lqr(discrete, scalar(1.0), r1);
  const double dare_value = (0.25 + std::sqrt(4.0625)) / 2.0;
  expect_close(discrete_lqr.value, {dare_value});
  expect_close(discrete_lqr.gain, {0.5 * dare_value / (1.0 + dare_value)});

  const auto crossed = control::lqr(continuous, q4, r1, scalar(0.5));
  const auto crossed_residual = continuous.a()
                                    .transpose(-2, -1)
                                    .matmul(crossed.value)
                                    .add(crossed.value.matmul(continuous.a()))
                                    .sub(crossed.value.matmul(continuous.b())
                                             .add(scalar(0.5))
                                             .matmul(continuous.b()
                                                         .transpose(-2, -1)
                                                         .matmul(crossed.value)
                                                         .add(scalar(0.5))))
                                    .add(q4);
  expect(crossed_residual.abs().amax().item<double>() < 2e-8,
         "cross-weight CARE residual is too large");

  const auto estimator = control::lqe(continuous, q4, r1);
  expect_close(estimator.covariance, {care_value});
  expect_close(estimator.gain, {care_value});
  expect_close(estimator.closed_loop, {-2.0 - care_value});

  const auto oscillator = lti::state(
      pluss::series::make({{0.0, 1.0}, {-2.0, -3.0}}),
      pluss::series::make(std::vector<double>{0.0, 1.0}, {2, 1}),
      pluss::series::make({{1.0, 0.0}}), pluss::series::zeros({1, 1}));
  const auto identity2 = pluss::series::eye(2, pluss::ScalarType::Float64);
  const auto matrix_lqr = control::lqr(oscillator, identity2, r1);
  const auto matrix_residual =
      oscillator.a()
          .transpose(-2, -1)
          .matmul(matrix_lqr.value)
          .add(matrix_lqr.value.matmul(oscillator.a()))
          .sub(matrix_lqr.value.matmul(oscillator.b())
                   .matmul(oscillator.b().transpose(-2, -1))
                   .matmul(matrix_lqr.value))
          .add(identity2);
  expect(matrix_residual.abs().amax().item<double>() < 3e-8,
         "matrix CARE residual is too large");
  expect(
      matrix_lqr.value.allclose(matrix_lqr.value.transpose(-2, -1), 2e-9, 2e-9),
      "matrix CARE result lost symmetry");

  const auto discrete_oscillator = lti::state(
      pluss::series::make({{1.0, 0.1}, {0.0, 0.9}}),
      pluss::series::make(std::vector<double>{0.0, 0.1}, {2, 1}),
      pluss::series::make({{1.0, 0.0}}), pluss::series::zeros({1, 1}), 0.1);
  const auto matrix_dlqr = control::lqr(discrete_oscillator, identity2, r1);
  const auto gain_weight = r1.add(discrete_oscillator.b()
                                      .transpose(-2, -1)
                                      .matmul(matrix_dlqr.value)
                                      .matmul(discrete_oscillator.b()));
  const auto matrix_dare_residual =
      discrete_oscillator.a()
          .transpose(-2, -1)
          .matmul(matrix_dlqr.value)
          .matmul(discrete_oscillator.a())
          .sub(matrix_dlqr.value)
          .sub(discrete_oscillator.a()
                   .transpose(-2, -1)
                   .matmul(matrix_dlqr.value)
                   .matmul(discrete_oscillator.b())
                   .matmul(gain_weight.linearalg.inverse())
                   .matmul(discrete_oscillator.b().transpose(-2, -1))
                   .matmul(matrix_dlqr.value)
                   .matmul(discrete_oscillator.a()))
          .add(identity2);
  expect(matrix_dare_residual.abs().amax().item<double>() < 5e-8,
         "matrix DARE residual is too large");

  const auto float_system =
      lti::state(pluss::series::make(std::vector<float>{-2.0F}, {1, 1}),
                 pluss::series::make(std::vector<float>{1.0F}, {1, 1}),
                 pluss::series::make(std::vector<float>{1.0F}, {1, 1}),
                 pluss::series::make(std::vector<float>{0.0F}, {1, 1}));
  const auto float_lqr = control::lqr(
      float_system, pluss::series::make(std::vector<float>{4.0F}, {1, 1}),
      pluss::series::make(std::vector<float>{1.0F}, {1, 1}));
  expect(float_lqr.value.scalar_type() == pluss::ScalarType::Float32,
         "Float32 control problem promoted unexpectedly");
  expect_close(float_lqr.value, {care_value}, 3e-5);

  const auto filter_system =
      lti::state(scalar(1.0), scalar(0.5), scalar(1.0), scalar(2.0), 0.1);
  const auto filter_result = control::kalman(
      filter_system, pluss::series::make({2.0, 2.0, 2.0}).reshape({3, 1}),
      scalar(0.0), scalar(1.0),
      pluss::series::zeros({1}, pluss::ScalarType::Float64), scalar(1.0),
      {.jitter = 0.0});
  expect(filter_result.filtered_mean.sizes() == std::vector<std::int64_t>{3, 1},
         "engineering Kalman trajectory shape is wrong");
  expect_close(filter_result.filtered_mean.select(0, 0), {1.0});

  const auto controlled_filter = control::kalman(
      filter_system, pluss::series::make({2.5, 5.5}).reshape({2, 1}),
      pluss::series::make({1.0, 2.0}).reshape({2, 1}), scalar(0.0),
      scalar(1e-6), pluss::series::zeros({1}, pluss::ScalarType::Float64),
      scalar(1.0), {.jitter = 0.0});
  expect_close(controlled_filter.predicted_obs, {2.5, 5.5}, 2e-6);

  const double missing = std::numeric_limits<double>::quiet_NaN();
  const auto batched_filter = control::kalman(
      filter_system,
      pluss::series::make(
          std::vector<double>{1.0, missing, 1.0, missing, 1.0, 1.0}, {2, 3, 1}),
      scalar(0.01), scalar(0.25),
      pluss::series::zeros({1}, pluss::ScalarType::Float64), scalar(1.0));
  expect(batched_filter.filtered_mean.sizes() ==
             std::vector<std::int64_t>{2, 3, 1},
         "engineering Kalman batch shape is wrong");
  expect(batched_filter.filtered_mean.isfinite().all().item<bool>(),
         "missing sensor contaminated an engineering filter lane");

  const auto smooth_result = control::smooth(
      filter_system, pluss::series::make({0.2, 0.1, -0.3, 0.4}).reshape({4, 1}),
      scalar(0.04), scalar(0.25),
      pluss::series::zeros({1}, pluss::ScalarType::Float64), scalar(1.0),
      {.jitter = 0.0});
  expect_close(
      smooth_result.smoothed_mean.select(-2, 3),
      smooth_result.filter.filtered_mean.select(-2, 3).to_vec<double>());
  expect(
      smooth_result.smoothed_cov.le(smooth_result.filter.filtered_cov.add(1e-8))
          .all()
          .item<bool>(),
      "engineering smoother increased scalar uncertainty");

  auto tracked_observations = pluss::series::make({0.5, 0.8}).reshape({2, 1});
  tracked_observations.tracks_slope_(true);
  control::kalman(filter_system, tracked_observations, scalar(0.04),
                  scalar(0.25),
                  pluss::series::zeros({1}, pluss::ScalarType::Float64),
                  scalar(1.0), {.jitter = 0.0})
      .filtered_mean.sum()
      .reverse();
  expect(tracked_observations.sensitivity().defined() &&
             tracked_observations.sensitivity().isfinite().all().item<bool>(),
         "engineering Kalman observations lost reverse history");

  expect_error([&] { (void)scalar(1.1).linearalg.dlyap(scalar(1.0)); },
               "unstable discrete Lyapunov equation did not fail");
  expect_error([&] { (void)control::lqr(continuous, q4, scalar(0.0)); },
               "singular control weight was accepted");
  expect_error([&] { (void)control::lqr(continuous, q4, r1, scalar(3.0)); },
               "indefinite joint state-input cost was accepted");
  expect_error(
      [&] {
        (void)pluss::series::make({{-1.0, 0.0}, {0.0, -2.0}})
            .linearalg.lyap(pluss::series::make({{1.0, 2.0}, {0.0, 1.0}}));
      },
      "non-Hermitian Lyapunov driver was accepted");
  expect_error(
      [&] {
        (void)scalar(-2.0).linearalg.lyap(scalar(1.0), {.max_iterations = 0});
      },
      "invalid matrix-equation iteration policy was accepted");
  expect_error(
      [&] {
        (void)control::kalman(
            continuous, pluss::series::make({1.0}).reshape({1, 1}), scalar(0.1),
            scalar(0.2), pluss::series::zeros({1}, pluss::ScalarType::Float64),
            scalar(1.0));
      },
      "continuous state-space model was accepted by the discrete filter");
}
