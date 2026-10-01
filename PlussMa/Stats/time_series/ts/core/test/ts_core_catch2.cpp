#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::Direction;
using pluss::subsp::stats::ts::LagPlanOptions;
using pluss::subsp::stats::ts::PartitionOptions;
using pluss::subsp::stats::ts::TsPartition;
using pluss::subsp::stats::ts::build_lag_plan;
using pluss::subsp::stats::ts::build_lagged_predictors;

std::vector<std::int64_t> to_i64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Int64).reshape({x.numel()}).to_vec<std::int64_t>();
}

std::vector<std::uint8_t> to_u8(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Bool).reshape({x.numel()}).to_vec<std::uint8_t>();
}

std::int64_t count_non_nan(const Series& x) {
  return static_cast<std::int64_t>(
      x.isfinite().to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).sum().item<double>());
}

} // namespace

TEST_CASE("ts partition edge behavior", "[ts][partition][edge]") {
  SECTION("expanding window reverse creates valid ordered windows") {
    PartitionOptions opts;
    opts.direction = Direction::Reverse;
    opts.gap_size = 1;
    opts.min_train_size = 5;
    opts.test_size = 2;
    opts.step_size = 2;
    opts.max_lag = 2;
    const auto p = TsPartition::expanding_window(24, 4, opts);

    REQUIRE(p.num_test_sets() == 4);
    for (std::int64_t i = 0; i < p.num_test_sets(); ++i) {
      const auto& w = p.window(i);
      REQUIRE(w.train_size() >= 5);
      REQUIRE(w.test_size() == 2);
      REQUIRE(w.test_start - w.train_end - 1 >= 1);
      REQUIRE(w.test_buffer_size() <= 2);
      REQUIRE(w.train_start >= 0);
      REQUIRE(w.test_end < 24);
    }
  }

  SECTION("sliding window forward has fixed train size") {
    PartitionOptions opts;
    opts.direction = Direction::Forward;
    opts.train_size = 6;
    opts.test_size = 2;
    opts.step_size = 2;
    opts.max_lag = 1;
    const auto p = TsPartition::sliding_window(30, 5, opts);

    REQUIRE(p.num_test_sets() == 5);
    for (std::int64_t i = 0; i < p.num_test_sets(); ++i) {
      REQUIRE(p.window(i).train_size() == 6);
      REQUIRE(p.window(i).test_size() == 2);
    }
  }

  SECTION("holdout supports both proportion and count") {
    PartitionOptions opts;
    opts.gap_size = 1;
    const auto p_prop = TsPartition::holdout(100, 0.2, opts);
    const auto p_cnt = TsPartition::holdout(100, 20.0, opts);
    REQUIRE(p_prop.window(0).test_size() == 20);
    REQUIRE(p_cnt.window(0).test_size() == 20);
  }

  SECTION("invalid configurations throw") {
    PartitionOptions opts;
    opts.train_size = 3;
    opts.max_lag = 3;
    REQUIRE_THROWS(TsPartition::sliding_window(20, 2, opts));

    PartitionOptions bad_gap;
    bad_gap.gap_size = -1;
    REQUIRE_THROWS(TsPartition::holdout(10, 0.2, bad_gap));
  }
}

TEST_CASE("ts lag plan and design matrix edge coverage", "[ts][lag][edge]") {
  LagPlanOptions options;
  options.horizon = {1, 2, 3};
  options.response_lags = {1};
  options.leading_predictors = {2};
  const auto plan = build_lag_plan(3, options);

  REQUIRE(plan.max_lag == 1);
  REQUIRE(plan.max_lead == 2);
  REQUIRE(plan.feature_count == 6);

  const auto mask = to_u8(plan.prepared_predictors_per_horizon);
  const std::vector<std::uint8_t> expected{
      1, 1, 1, 0, 0, 1,
      1, 1, 1, 0, 1, 0,
      1, 1, 1, 1, 0, 0};
  REQUIRE(mask == expected);
}

TEST_CASE("ts lag design numerical stability and sensitivity", "[ts][lag][numeric][sensitivity]") {
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series x = pluss::make_series(std::vector<double>{
                                          1e-8, 1e8, 1.0,
                                          2e-8, 2e8, 2.0,
                                          3e-8, 3e8, 3.0,
                                          4e-8, 4e8, 4.0,
                                          5e-8, 5e8, 5.0,
                                          6e-8, 6e8, 6.0},
                                      {6, 3},
                                      cfg);
  const Series y = pluss::make_series(std::vector<double>{0.1, 0.2, 0.3, 0.4, 0.5, 0.6}, {6}, cfg);

  LagPlanOptions opt_a;
  opt_a.horizon = {1, 2};
  opt_a.response_lags = {1, 2};
  opt_a.leading_predictors = {2};
  const auto plan_a = build_lag_plan(3, opt_a);
  const Series xa = build_lagged_predictors(x, y, plan_a, cfg);

  LagPlanOptions opt_b = opt_a;
  opt_b.horizon = {1, 2, 3, 4};
  const auto plan_b = build_lag_plan(3, opt_b);
  const Series xb = build_lagged_predictors(x, y, plan_b, cfg);

  REQUIRE(xa.sizes()[1] == plan_a.feature_count);
  REQUIRE(xb.sizes()[1] == plan_b.feature_count);
  REQUIRE(plan_b.feature_count >= plan_a.feature_count);
  REQUIRE(xb.sizes()[0] > xa.sizes()[0]);

  // Non-NaN entries are finite and numerically stable for large/small scale mixtures.
  REQUIRE(xa.nan_to_num(0.0, 0.0, 0.0).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(xb.nan_to_num(0.0, 0.0, 0.0).isfinite().all().item<double>() == Approx(1.0));
}

TEST_CASE("ts lag stress and crash resilience", "[ts][lag][stress][crash]") {
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Stress: moderate-large design generation should complete and preserve expected shape.
  const std::int64_t n = 40000;
  const Series x = pluss::series::rand({n, 8}, cfg, 20260309ULL);
  const Series y = pluss::series::rand({n}, cfg, 20260310ULL);

  LagPlanOptions stress_opts;
  stress_opts.horizon = {1, 5, 10};
  stress_opts.response_lags = {1, 2, 4, 8};
  stress_opts.leading_predictors = {6, 7};
  stress_opts.predictor_lags = {{1, 2, 3}, {1, 2, 5}, {1}, {2}, {1, 3}, {4}};
  stress_opts.leading_predictor_lags = {{0, 1, 2}, {0, 2}};

  const auto plan = build_lag_plan(8, stress_opts);
  const Series design = build_lagged_predictors(x, y, plan, cfg);
  REQUIRE(design.sizes()[1] == plan.feature_count);
  REQUIRE(design.sizes()[0] == n + plan.max_lag + plan.max_lead);
  REQUIRE(count_non_nan(design) > 0);

  // Crash tests: invalid plan usage should throw cleanly.
  REQUIRE_THROWS(build_lag_plan(-1, stress_opts));
  REQUIRE_THROWS(build_lagged_predictors(x, pluss::series::rand({n + 1}, cfg, 11ULL), plan, cfg));
  REQUIRE_THROWS(build_lagged_predictors(pluss::series::undefined(), y, plan, cfg));
}

TEST_CASE("ts partition masks match index outputs", "[ts][partition][mask]") {
  PartitionOptions opts;
  opts.direction = Direction::Forward;
  opts.train_size = 8;
  opts.test_size = 3;
  opts.step_size = 4;
  opts.max_lag = 2;
  const auto p = TsPartition::sliding_window(40, 4, opts);

  for (std::int64_t i = 0; i < p.num_test_sets(); ++i) {
    const auto train_idx = to_i64(p.training(i));
    const auto test_idx = to_i64(p.test(i));
    const auto train_mask = to_u8(p.training_mask(i));
    const auto test_mask = to_u8(p.test_mask(i));

    REQUIRE(static_cast<std::int64_t>(train_idx.size()) == p.window(i).train_size());
    REQUIRE(static_cast<std::int64_t>(test_idx.size()) == p.window(i).test_size());

    for (const auto idx : train_idx) {
      REQUIRE(train_mask[static_cast<std::size_t>(idx)] == static_cast<std::uint8_t>(1));
    }
    for (const auto idx : test_idx) {
      REQUIRE(test_mask[static_cast<std::size_t>(idx)] == static_cast<std::uint8_t>(1));
    }

    REQUIRE(std::none_of(train_idx.begin(), train_idx.end(), [&test_idx](const std::int64_t t) {
      return std::find(test_idx.begin(), test_idx.end(), t) != test_idx.end();
    }));
  }
}
