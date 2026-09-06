#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64))},
      {"balance", Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64))},
      {"target", Thread(pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64))},
  });

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  const workflow::Workflow trained = wf.learn(raw, data_options);

  // Once the routed scaler stage is fitted (via learn), map replays only
  // the preparation -- no estimator inference -- and returns the
  // transformed feature matrix, useful for inspecting what the model
  // actually sees.
  const pluss::Series prepared = trained.map(raw, data_options);
  std::cout << "map -> " << prepared << "\n";

  return 0;
}
