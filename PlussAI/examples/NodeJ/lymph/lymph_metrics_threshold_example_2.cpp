#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Threshold::best(objective) returns the single cutoff that maximizes the given
// objective over the full sweep. Precision, Recall and F1 typically land on
// different thresholds.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5,
       6.0, 6.5, 7.0, 7.5, 8.0, 8.5}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Thread(x)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  const lymph::models::Model model =
      lymph::models::logistic_regression().learn(frame, data_options);
  const pluss::Series scores = model.infer_scores(frame, data_options);

  const lymph::metrics::classification::Threshold th =
      lymph::metrics::classification::threshold(scores, frame.features("target").as_series());

  std::cout << "best for F1        = " << th.best(lymph::MetricKind::F1) << "\n";
  std::cout << "best for Precision = " << th.best(lymph::MetricKind::Precision) << "\n";
  std::cout << "best for Recall    = " << th.best(lymph::MetricKind::Recall) << "\n";

  return 0;
}
