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
  });

  // learn_map fits the routed prep stage(s) and returns the mapped output in
  // one call -- it does NOT fit the estimator, so trained() stays false
  // afterward. This is the entry point for a purely transform-style model
  // (a scaler/encoder pipeline, or an unsupervised model such as PCA) where
  // there is no label to fit an estimator against.
  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  const pluss::Series mapped = wf.learn_map(raw);
  std::cout << "learn_map -> " << mapped << "\n";
  std::cout << "stage_count=" << wf.stage_count() << " trained=" << std::boolalpha << wf.trained() << "\n";

  return 0;
}
