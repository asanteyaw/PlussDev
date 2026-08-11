#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const Series x = pluss::series::linspace(-3.0, 3.0, 84, ScalarType::Float64);
  const Series score = x.mul(1.8).add(x.sin());
  const DataFrame raw({{"x", Thread(x)}, {"score", Thread(score)}, {"target", Thread(score.gt(0.4).to(ScalarType::Float64))}});
  lymph::Payload data = lymph::payload::make(raw).features({"x", "score"}).label("target");

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"x", "score"}, {{"scale", lymph::stage::standard_scaler()}}),
      },
      lymph::models::logistic_regression({.regularization_strength = 0.01}));
  wf.learn_(data);

  const lymph::ThresholdCurve curve = lymph::reports::threshold_curve(wf, data);
  const lymph::ClassReport summary = lymph::reports::classification_summary(wf, data);
  const double threshold = lymph::reports::best_threshold(wf, data, lymph::MetricKind::F1);

  std::cout << "threshold points: " << curve.thresholds.numel() << "\n";
  std::cout << "best threshold: " << threshold << "\n";
  std::cout << "class rows: " << summary.labels.numel() << "\n";
  return 0;
}
