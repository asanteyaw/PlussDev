#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The workflow overload and the bound accessor: both threshold helpers work
// on a Workflow + Payload, and on model.reports() directly.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"id", Slate(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {10}, f64))},
      {"income", Slate(pluss::make_series(
          {1.0, 1.3, 1.7, 2.1, 2.4, 2.8, 3.2, 3.6, 4.0, 4.4}, {10}, f64))},
      {"balance", Slate(pluss::make_series(
          {0.4, 0.7, 1.0, 1.2, 1.5, 1.8, 2.1, 2.4, 2.7, 3.0}, {10}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"id"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(raw, data_options);

  const workflow::Payload payload =
      workflow::payload::make(raw).features({"income", "balance"}).label("default");

  const double wf_best = lymph::reports::best_threshold(trained, payload, lymph::MetricKind::F1);
  std::cout << "workflow best_threshold(F1) = " << wf_best << "\n";

  // The bound accessor on a plain model:
  lymph::models::Model model = lymph::models::logistic_regression();
  const DataFrame x({{"income", raw.features("income")}, {"balance", raw.features("balance")}});
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  model.learn_(x, y);
  const lymph::ThresholdCurve curve = model.reports().threshold_curve(x, y);
  std::cout << "accessor threshold count = " << curve.thresholds.numel() << "\n";
  std::cout << "accessor best_threshold(F1) = "
            << model.reports().best_threshold(x, y, lymph::MetricKind::F1) << "\n";

  return 0;
}
