#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The workflow overload: reports::classification_summary(workflow, payload)
// runs the Workflow (Policy + prep + model) then scores -- the same evidence
// for a whole deployable artifact, not just a bare model. And
// classification_metric_series(...) returns the scalar metrics as one Series.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"id", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.2, 1.4, 1.1, 3.0, 3.2, 3.4, 3.1}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.5, 0.7, 0.6, 0.8, 2.5, 2.7, 2.6, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
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

  const lymph::ClassReport report = lymph::reports::classification_summary(trained, payload);
  std::cout << "workflow accuracy = " << report.accuracy.item<double>() << "\n";
  std::cout << "workflow f1       =\n" << report.f1 << "\n";

  const pluss::Series metric_series =
      lymph::reports::classification_metric_series(trained, payload);
  std::cout << "metric_series =\n" << metric_series << "\n";

  return 0;
}
