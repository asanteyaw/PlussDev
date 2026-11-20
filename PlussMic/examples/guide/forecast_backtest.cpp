// A forecasting baseline with an honest backtest: build lag features, split by time (never at random), fit on the past,
// score on the future, and compare a regression with the "same month last year" rule.
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

using pluss::ScalarType;
using pluss::table::DataFrame;

int main() {
  // Three years of monthly demand: a rising trend, a yearly cycle and noise.
  const std::int64_t months = 36;
  auto t = pluss::series::arange(0, months, ScalarType::Float64);
  auto demand = t.mul(2.0).add(200.0).add(t.mul(2.0 * 3.14159265358979 / 12.0).sin().mul(30.0)).add(pluss::series::randn({months}, ScalarType::Float64, 12).mul(5.0));
  auto frame = DataFrame({{"month", pluss::series::arange(0, months, ScalarType::Int64)}, {"demand", demand}});

  // 1. Lag features: demand one month ago and twelve months ago. The first twelve rows have no twelve-month lag, so they are cut.
  auto with_lags = frame.assign({{"lag1", [](const DataFrame &f) { return f.features("demand").ts().lag(1); }},
                                 {"lag12", [](const DataFrame &f) { return f.features("demand").ts().lag(12); }}});
  auto usable = with_lags.slice(12, months);
  std::cout << "usable rows " << usable.ncases() << " (of " << months << "), features " << usable.nfeatures() << "\n";

  // 2. Split by time: train on months 12-23, test on months 24-35. Shuffling would leak the future into the training set.
  auto train = usable.slice(0, 12);
  auto test = usable.slice(12, 24);
  std::cout << "train months " << train.features("month").as_series().min().item<double>() << " to " << train.features("month").as_series().max().item<double>()
            << ", test months " << test.features("month").as_series().min().item<double>() << " to " << test.features("month").as_series().max().item<double>() << "\n";

  // 3. Fit demand ~ 1 + lag1 + lag12 on the training rows by least squares.
  const auto design = [](const DataFrame &d) {
    auto ones = pluss::series::ones({d.ncases(), 1}, ScalarType::Float64);
    return ones.unite({d.features("lag1").as_series().unsqueeze(1), d.features("lag12").as_series().unsqueeze(1)}, 1);
  };
  auto x = design(train);
  auto y = train.features("demand").as_series().unsqueeze(1);
  auto beta = x.t().matmul(x).linearalg().inverse().matmul(x.t().matmul(y));
  std::cout << "coefficients (intercept, lag1, lag12): " << pluss::series::print(beta.squeeze(1)) << "\n";

  // 4. Forecast the test months two ways and score both: mean absolute error and mean absolute percentage error.
  auto actual = test.features("demand").as_series();
  auto regression = design(test).matmul(beta).squeeze(1);
  auto seasonal_naive = test.features("lag12").as_series();
  const auto score = [&](const char *name, const pluss::Series &forecast) {
    auto error = forecast.sub(actual).abs();
    std::cout << name << ": MAE " << error.mean().item<double>() << ", MAPE " << 100.0 * error.div(actual).mean().item<double>() << "%\n";
  };
  score("same month last year", seasonal_naive);
  score("regression on lag1, lag12", regression);
}
