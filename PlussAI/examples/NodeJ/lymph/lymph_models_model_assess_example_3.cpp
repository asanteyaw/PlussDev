#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // A regression target: roughly 2*hours + 1 with mild noise.
  const pluss::Series hours = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {8}, f64);
  const pluss::Series score = pluss::make_series({3.2, 4.8, 7.1, 8.9, 11.2, 12.8, 15.3, 16.9}, {8}, f64);
  const DataFrame x_train({{"hours", Slate(hours)}});

  const lymph::models::Model model = lymph::models::linear_regression().learn(x_train, score);

  // For a regressor, assess reports error metrics instead of classification
  // ones. It does NOT hold anything out -- these numbers are the fit on the
  // rows it just learned from.
  lymph::options::Assess options{};
  options.metrics = {lymph::MetricKind::Rmse, lymph::MetricKind::Mae, lymph::MetricKind::R2};
  const lymph::MetricReport on_train = model.assess(x_train, score, options);

  // The same model measured against rows it never saw.
  const pluss::Series holdout_hours = pluss::make_series({2.5, 5.5, 9.0}, {3}, f64);
  const pluss::Series holdout_score = pluss::make_series({6.0, 11.8, 19.2}, {3}, f64);
  const DataFrame x_holdout({{"hours", Slate(holdout_hours)}});
  const lymph::MetricReport on_holdout = model.assess(x_holdout, holdout_score, options);

  std::cout << "on training rows:  rmse = " << on_train.rmse.item<double>()
            << ", mae = " << on_train.mae.item<double>()
            << ", r2 = " << on_train.r2.item<double>() << "\n";
  std::cout << "on held-out rows:  rmse = " << on_holdout.rmse.item<double>()
            << ", mae = " << on_holdout.mae.item<double>()
            << ", r2 = " << on_holdout.r2.item<double>() << "\n";

  return 0;
}
