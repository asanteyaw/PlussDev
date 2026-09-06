#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The practical loop: pick the F1-optimal threshold from held scores, then feed
// it back as InferOptions.positive_threshold so inference labels at that cutoff
// instead of the default 0.5.
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

  const double tuned =
      lymph::metrics::classification::threshold(scores, frame.features("target").as_series())
          .best(lymph::MetricKind::F1);
  std::cout << "tuned threshold = " << tuned << "\n";

  lymph::InferOptions tuned_infer{};
  tuned_infer.positive_threshold = tuned;

  const pluss::Series at_default = model.infer(frame.features({"x"}));
  const pluss::Series at_tuned = model.infer(frame.features({"x"}), tuned_infer);
  std::cout << "labels @ 0.5   = " << at_default << "\n";
  std::cout << "labels @ tuned = " << at_tuned << "\n";

  return 0;
}
