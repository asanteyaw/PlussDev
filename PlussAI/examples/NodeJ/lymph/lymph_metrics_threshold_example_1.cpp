#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// metrics::classification::threshold(scores, labels).curve() sweeps every achievable decision
// threshold in one pass and returns aligned thresholds / precision / recall /
// f1 Series (thresholds descending).
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

  const lymph::ThresholdCurve curve =
      lymph::metrics::classification::threshold(scores, frame.features("target").as_series()).curve();

  std::cout << "points     = " << curve.thresholds.numel() << "\n";
  std::cout << "thresholds = " << curve.thresholds.slice(0, 0, 4) << "\n";
  std::cout << "precision  = " << curve.precision.slice(0, 0, 4) << "\n";
  std::cout << "recall     = " << curve.recall.slice(0, 0, 4) << "\n";
  std::cout << "f1         = " << curve.f1.slice(0, 0, 4) << "\n";

  return 0;
}
