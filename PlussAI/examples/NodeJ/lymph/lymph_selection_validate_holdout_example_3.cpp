#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The practical use of a holdout: split once, train on train_set, judge on the
// untouched test_set. The split frames are ordinary DataFrames.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Slate(income)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income"};
  data_options.label_feature = "target";

  const lymph::prep::HoldoutSplit split =
      lymph::selection::holdout().test_size(0.3).stratify("target").seed(20260901ULL).split(frame);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(split.train_set, data_options);

  const lymph::metrics::MetricReport report = model.assess(
      split.test_set, data_options, {.metrics = {lymph::MetricKind::Accuracy}});

  std::cout << "train_rows = " << split.train_set.ncases()
            << ", test_rows = " << split.test_set.ncases() << "\n";
  std::cout << "held-out accuracy = " << report.accuracy.item<double>() << "\n";

  return 0;
}
