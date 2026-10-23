#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/state.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace state = pluss::subsp::math::state;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_close(const pluss::Series &actual,
                  const std::vector<double> &expected,
                  double tolerance = 2e-8) {
  const auto values = actual.to(pluss::ScalarType::Float64).to_vec<double>();
  expect(values.size() == expected.size(), "state-estimation size mismatch");
  for (std::size_t index = 0; index < values.size(); ++index) {
    const double scale =
        std::max({1.0, std::abs(values[index]), std::abs(expected[index])});
    expect(std::abs(values[index] - expected[index]) <= tolerance * scale,
           "state-estimation value mismatch at lane " + std::to_string(index) +
               ": got " + std::to_string(values[index]) + ", expected " +
               std::to_string(expected[index]));
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

state::LinearModel
scalar_model(pluss::ScalarType dtype = pluss::ScalarType::Float64) {
  const pluss::Config cfg = pluss::Config{}.with_dtype(dtype);
  return {
      .transition = pluss::series::ones({1, 1}, cfg),
      .observation = pluss::series::ones({1, 1}, cfg),
      .process_cov = pluss::series::zeros({1, 1}, cfg),
      .observation_cov = pluss::series::ones({1, 1}, cfg),
      .initial_mean = pluss::series::zeros({1}, cfg),
      .initial_cov = pluss::series::ones({1, 1}, cfg),
  };
}

} // namespace

int main() {
  const state::FilterOptions exact{.info_tol = 0.0, .jitter = 0.0};
  const auto first =
      state::filter(pluss::series::make({2.0}), scalar_model(), exact);
  expect_close(first.predicted_mean, {0.0});
  expect_close(first.filtered_mean, {1.0});
  expect_close(first.filtered_cov, {0.5});
  expect_close(first.gain, {0.5});
  expect_close(first.loglik_terms, {-0.5 * (std::log(2.0 * std::acos(-1.0)) +
                                            std::log(2.0) + 2.0)});

  state::LinearModel mimo{
      .transition = pluss::series::eye(2, pluss::ScalarType::Float64),
      .observation = pluss::series::eye(2, pluss::ScalarType::Float64),
      .process_cov = pluss::series::zeros({2, 2}, pluss::ScalarType::Float64),
      .observation_cov = pluss::series::eye(2, pluss::ScalarType::Float64),
      .initial_mean = pluss::series::zeros({2}, pluss::ScalarType::Float64),
      .initial_cov = pluss::series::eye(2, pluss::ScalarType::Float64),
  };
  const auto mimo_result = state::filter(
      pluss::series::make({2.0, 4.0}).reshape({1, 2}), mimo, exact);
  expect_close(mimo_result.filtered_mean, {1.0, 2.0});
  expect_close(mimo_result.gain, {0.5, 0.0, 0.0, 0.5});
  expect_close(mimo_result.filtered_cov, {0.5, 0.0, 0.0, 0.5});

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto batched =
      state::filter(pluss::series::make(std::vector<double>{2.0, nan, 2.0, 2.0,
                                                            nan, 2.0, nan, 2.0},
                                        {2, 4, 1}),
                    scalar_model(), exact);
  expect(batched.filtered_mean.sizes() == std::vector<std::int64_t>{2, 4, 1},
         "batched filter shape is wrong");
  expect_close(batched.data_used.to(pluss::ScalarType::Float64),
               {1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0});
  expect(batched.filtered_mean.isfinite().all().item<bool>(),
         "lane-specific missing data contaminated the state");
  expect_close(batched.filtered_mean.select(0, 0).select(0, 1), {1.0});
  expect_close(batched.predicted_mean.select(0, 0).select(0, 1), {1.0});

  const auto no_data =
      state::filter(pluss::series::make(std::vector<double>{nan, nan}, {2, 1}),
                    scalar_model(), exact);
  expect_close(no_data.filtered_mean, {0.0, 0.0});
  expect_close(no_data.loglik, {0.0});

  auto model_batch = scalar_model();
  model_batch.transition =
      pluss::series::make(std::vector<double>{1.0, 0.5}, {2, 1, 1});
  model_batch.process_cov =
      pluss::series::zeros({2, 1, 1}, pluss::ScalarType::Float64);
  model_batch.initial_mean =
      pluss::series::make(std::vector<double>{0.0, 2.0}, {2, 1, 1});
  model_batch.initial_cov =
      pluss::series::ones({2, 1, 1}, pluss::ScalarType::Float64);
  const auto model_batched = state::filter(
      pluss::series::make({1.0, 1.0}).reshape({2, 1}), model_batch, exact);
  expect(model_batched.filtered_mean.sizes() ==
             std::vector<std::int64_t>{2, 2, 1},
         "model batch axes were not retained");
  expect_close(model_batched.predicted_mean.select(0, 1), {1.0, 0.5});

  auto controlled_model = scalar_model();
  controlled_model.input = pluss::series::make({1.0, 2.0}).reshape({2, 1});
  controlled_model.state_input = pluss::series::make({0.5}).reshape({1, 1});
  controlled_model.obs_input = pluss::series::make({2.0}).reshape({1, 1});
  controlled_model.observation_cov = pluss::series::full(
      {1, 1}, 1e-6, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto controlled = state::filter(
      pluss::series::make({2.5, 6.0}).reshape({2, 1}), controlled_model, exact);
  expect_close(controlled.predicted_obs.select(0, 0), {2.5});
  expect_close(controlled.predicted_obs.select(0, 1), {5.5}, 2e-6);

  auto dynamic_model = scalar_model();
  dynamic_model.transition = pluss::series::make({0.9}).reshape({1, 1});
  dynamic_model.process_cov = pluss::series::make({0.04}).reshape({1, 1});
  dynamic_model.observation_cov = pluss::series::make({0.25}).reshape({1, 1});
  const auto smooth =
      state::smooth(pluss::series::make({0.4, -0.2, 0.7, 0.1}).reshape({4, 1}),
                    dynamic_model, exact);
  expect_close(smooth.smoothed_mean.select(-2, 3),
               smooth.filter.filtered_mean.select(-2, 3).to_vec<double>());
  expect_close(smooth.smoothed_cov.select(-3, 3),
               smooth.filter.filtered_cov.select(-3, 3).to_vec<double>());
  expect(smooth.smoothed_cov.le(smooth.filter.filtered_cov.add(1e-8))
             .all()
             .item<bool>(),
         "RTS smoothing increased scalar uncertainty");

  auto float_model = scalar_model(pluss::ScalarType::Float32);
  const pluss::Series float_observations = pluss::series::full(
      {3, 1}, 1.0, pluss::Config{}.with_dtype(pluss::ScalarType::Float32));
  const auto float_result = state::filter(float_observations, float_model);
  expect(float_result.filtered_mean.scalar_type() == pluss::ScalarType::Float32,
         "filter did not retain Float32");

  const pluss::Series padded =
      pluss::series::make({1.0, 99.0, 2.0, 99.0, 3.0, 99.0}).reshape({6, 1});
  const pluss::Series strided = padded.slice(0, 0, 6, 2);
  const auto strided_result = state::filter(strided, scalar_model(), exact);
  expect(strided_result.filtered_mean.size(-2) == 3,
         "strided observations lost their time extent");
  expect(strided_result.filtered_mean.isfinite().all().item<bool>(),
         "strided observations produced invalid state values");

  auto tracked_y = pluss::series::make({0.5, 0.8}).reshape({2, 1});
  auto tracked_a = pluss::series::make({0.9}).reshape({1, 1});
  tracked_y.tracks_slope_(true);
  tracked_a.tracks_slope_(true);
  auto tracked_model = dynamic_model;
  tracked_model.transition = tracked_a;
  state::filter(tracked_y, tracked_model, exact).filtered_mean.sum().reverse();
  expect(tracked_y.sensitivity().defined() &&
             tracked_y.sensitivity().isfinite().all().item<bool>(),
         "observation sensitivity is missing");
  expect(tracked_a.sensitivity().defined() &&
             tracked_a.sensitivity().isfinite().all().item<bool>(),
         "transition sensitivity is missing");

  auto likelihood_y = pluss::series::make({0.5, 0.8}).reshape({2, 1});
  auto likelihood_r = pluss::series::make({0.25}).reshape({1, 1});
  likelihood_y.tracks_slope_(true);
  likelihood_r.tracks_slope_(true);
  auto likelihood_model = dynamic_model;
  likelihood_model.observation_cov = likelihood_r;
  state::filter(likelihood_y, likelihood_model, exact).loglik.reverse();
  expect(likelihood_y.sensitivity().defined() &&
             likelihood_y.sensitivity().isfinite().all().item<bool>(),
         "likelihood observation sensitivity is missing");
  expect(likelihood_r.sensitivity().defined() &&
             likelihood_r.sensitivity().isfinite().all().item<bool>(),
         "likelihood covariance sensitivity is missing");

  auto bad_model = scalar_model();
  bad_model.observation_cov = pluss::series::make({-1.0}).reshape({1, 1});
  expect_error(
      [&] { (void)state::filter(pluss::series::make({1.0}), bad_model); },
      "negative covariance diagonal was accepted");
  auto indefinite_model = mimo;
  indefinite_model.observation_cov =
      pluss::series::make({1.0, 2.0, 2.0, 1.0}).reshape({2, 2});
  expect_error(
      [&] {
        (void)state::filter(pluss::series::make({1.0, 2.0}).reshape({1, 2}),
                            indefinite_model);
      },
      "indefinite covariance with a non-negative diagonal was accepted");
  expect_error(
      [&] {
        (void)state::filter(
            pluss::series::full(
                {2, 1}, 1.0,
                pluss::Config{}.with_dtype(pluss::ScalarType::Float32)),
            scalar_model());
      },
      "mixed observation/model dtype was accepted");

  return 0;
}
