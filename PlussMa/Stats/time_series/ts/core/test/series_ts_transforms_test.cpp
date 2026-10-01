#include <pluss/ma.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::GapFillMode;
using pluss::subsp::stats::ts::ResampleOp;

std::vector<double> to_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void expect_close(double got, double want, double tol, const std::string& message) {
  if (std::isnan(want)) {
    expect(std::isnan(got), message + " (expected NaN)");
    return;
  }
  expect(std::abs(got - want) <= tol, message + " (got=" + std::to_string(got) + ", want=" + std::to_string(want) + ")");
}

void test_lag_diff_returns() {
  const auto x = pluss::make_series(std::vector<double>{100.0, 110.0, 121.0, 133.1});

  const auto lagged = to_f64(pluss::subsp::stats::ts::lag(x, 1));
  expect_close(lagged[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "lag[0]");
  expect_close(lagged[1], 100.0, 1e-12, "lag[1]");
  expect_close(lagged[2], 110.0, 1e-12, "lag[2]");
  expect_close(lagged[3], 121.0, 1e-12, "lag[3]");

  const auto diff = to_f64(pluss::subsp::stats::ts::diff(x, 1));
  expect_close(diff[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "diff[0]");
  expect_close(diff[1], 10.0, 1e-12, "diff[1]");
  expect_close(diff[2], 11.0, 1e-12, "diff[2]");
  expect_close(diff[3], 12.1, 1e-12, "diff[3]");

  const auto ret = to_f64(pluss::subsp::stats::ts::returns(x, 1));
  expect_close(ret[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "returns[0]");
  expect_close(ret[1], 0.1, 1e-12, "returns[1]");
  expect_close(ret[2], 0.1, 1e-12, "returns[2]");
  expect_close(ret[3], 0.1, 1e-12, "returns[3]");

  const auto lret = to_f64(pluss::subsp::stats::ts::log_returns(x, 1));
  const double lr = std::log(1.1);
  expect_close(lret[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "log_returns[0]");
  expect_close(lret[1], lr, 1e-12, "log_returns[1]");
  expect_close(lret[2], lr, 1e-12, "log_returns[2]");
  expect_close(lret[3], lr, 1e-12, "log_returns[3]");
}

void test_rolling_and_ema() {
  const auto x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0});

  const auto mean = to_f64(pluss::subsp::stats::ts::rolling_mean(x, 2, 2));
  expect_close(mean[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "rolling_mean[0]");
  expect_close(mean[1], 1.5, 1e-12, "rolling_mean[1]");
  expect_close(mean[2], 2.5, 1e-12, "rolling_mean[2]");
  expect_close(mean[3], 3.5, 1e-12, "rolling_mean[3]");

  const auto stdv = to_f64(pluss::subsp::stats::ts::rolling_std(x, 2, 2));
  expect_close(stdv[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "rolling_std[0]");
  expect_close(stdv[1], 0.5, 1e-12, "rolling_std[1]");
  expect_close(stdv[2], 0.5, 1e-12, "rolling_std[2]");
  expect_close(stdv[3], 0.5, 1e-12, "rolling_std[3]");

  const auto sumv = to_f64(pluss::subsp::stats::ts::rolling_sum(x, 2, 2));
  expect_close(sumv[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "rolling_sum[0]");
  expect_close(sumv[1], 3.0, 1e-12, "rolling_sum[1]");
  expect_close(sumv[2], 5.0, 1e-12, "rolling_sum[2]");
  expect_close(sumv[3], 7.0, 1e-12, "rolling_sum[3]");

  const auto varv = to_f64(pluss::subsp::stats::ts::rolling_var(x, 2, 2));
  expect_close(varv[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "rolling_var[0]");
  expect_close(varv[1], 0.25, 1e-12, "rolling_var[1]");
  expect_close(varv[2], 0.25, 1e-12, "rolling_var[2]");
  expect_close(varv[3], 0.25, 1e-12, "rolling_var[3]");

  const auto minv = to_f64(pluss::subsp::stats::ts::rolling_min(x, 2, 2));
  expect_close(minv[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "rolling_min[0]");
  expect_close(minv[1], 1.0, 1e-12, "rolling_min[1]");
  expect_close(minv[2], 2.0, 1e-12, "rolling_min[2]");
  expect_close(minv[3], 3.0, 1e-12, "rolling_min[3]");

  const auto maxv = to_f64(pluss::subsp::stats::ts::rolling_max(x, 2, 2));
  expect_close(maxv[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "rolling_max[0]");
  expect_close(maxv[1], 2.0, 1e-12, "rolling_max[1]");
  expect_close(maxv[2], 3.0, 1e-12, "rolling_max[2]");
  expect_close(maxv[3], 4.0, 1e-12, "rolling_max[3]");

  const auto ema = to_f64(pluss::subsp::stats::ts::ema(x, 0.5));
  expect_close(ema[0], 1.0, 1e-12, "ema[0]");
  expect_close(ema[1], 1.5, 1e-12, "ema[1]");
  expect_close(ema[2], 2.25, 1e-12, "ema[2]");
  expect_close(ema[3], 3.125, 1e-12, "ema[3]");

  const auto ewm_std = to_f64(pluss::subsp::stats::ts::ewm_std(x, 0.5));
  expect(ewm_std.size() == 4, "ewm_std shape");
  expect(std::isfinite(ewm_std[3]), "ewm_std finite");

  const auto exp_mean = to_f64(pluss::subsp::stats::ts::expanding_mean(x, 2));
  expect_close(exp_mean[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "expanding_mean[0]");
  expect_close(exp_mean[1], 1.5, 1e-12, "expanding_mean[1]");
  expect_close(exp_mean[2], 2.0, 1e-12, "expanding_mean[2]");
  expect_close(exp_mean[3], 2.5, 1e-12, "expanding_mean[3]");

  const auto exp_std = to_f64(pluss::subsp::stats::ts::expanding_std(x, 2));
  expect_close(exp_std[0], std::numeric_limits<double>::quiet_NaN(), 0.0, "expanding_std[0]");
  expect(exp_std[1] > 0.0, "expanding_std[1]");
  expect(exp_std[2] > exp_std[1], "expanding_std monotonic check");
}

void test_fill_gaps_and_resample() {
  const auto x = pluss::make_series(std::vector<double>{
      1.0,
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::quiet_NaN(),
      4.0,
      std::numeric_limits<double>::quiet_NaN(),
  });

  const auto fwd = to_f64(pluss::subsp::stats::ts::fill_gaps(x, GapFillMode::Forward));
  expect_close(fwd[0], 1.0, 1e-12, "fill_fwd[0]");
  expect_close(fwd[1], 1.0, 1e-12, "fill_fwd[1]");
  expect_close(fwd[2], 1.0, 1e-12, "fill_fwd[2]");
  expect_close(fwd[3], 4.0, 1e-12, "fill_fwd[3]");
  expect_close(fwd[4], 4.0, 1e-12, "fill_fwd[4]");

  const auto bwd = to_f64(pluss::subsp::stats::ts::fill_gaps(x, GapFillMode::Backward));
  expect_close(bwd[0], 1.0, 1e-12, "fill_bwd[0]");
  expect_close(bwd[1], 4.0, 1e-12, "fill_bwd[1]");
  expect_close(bwd[2], 4.0, 1e-12, "fill_bwd[2]");
  expect_close(bwd[3], 4.0, 1e-12, "fill_bwd[3]");
  expect_close(bwd[4], std::numeric_limits<double>::quiet_NaN(), 0.0, "fill_bwd[4]");

  const auto cst = to_f64(pluss::subsp::stats::ts::fill_gaps(x, GapFillMode::Constant, 0.0));
  expect_close(cst[0], 1.0, 1e-12, "fill_const[0]");
  expect_close(cst[1], 0.0, 1e-12, "fill_const[1]");
  expect_close(cst[2], 0.0, 1e-12, "fill_const[2]");
  expect_close(cst[3], 4.0, 1e-12, "fill_const[3]");
  expect_close(cst[4], 0.0, 1e-12, "fill_const[4]");

  const std::vector<std::string_view> iso{
      "2024-01-01T00:00:00Z",
      "2024-01-01T12:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-02T12:00:00Z",
  };
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(iso.size())};
  const auto t = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(iso.data(), iso.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  const auto values = pluss::make_series(std::vector<double>{1.0, 3.0, 2.0, 4.0});
  const auto out_mean = pluss::subsp::stats::ts::resample(t, values, "D", ResampleOp::Mean, 1);
  expect(out_mean.labels.has_value(), "resample mean should produce labels");
  const auto mean_vals = to_f64(out_mean.values);
  expect(mean_vals.size() == 2, "resample mean bin count");
  expect_close(mean_vals[0], 2.0, 1e-12, "resample mean day1");
  expect_close(mean_vals[1], 3.0, 1e-12, "resample mean day2");

  const auto out_count = pluss::subsp::stats::ts::resample(t, values, "D", ResampleOp::Count, 1);
  expect(out_count.labels.has_value(), "resample count should produce labels");
  const auto count_vals = to_f64(out_count.values);
  expect(count_vals.size() == 2, "resample count bin count");
  expect_close(count_vals[0], 2.0, 1e-12, "resample count day1");
  expect_close(count_vals[1], 2.0, 1e-12, "resample count day2");
}

} // namespace

int main() {
  try {
    test_lag_diff_returns();
    test_rolling_and_ema();
    test_fill_gaps_and_resample();
    std::cout << "series_ts_transforms_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "series_ts_transforms_test failed: " << e.what() << '\n';
    return 1;
  }
}
