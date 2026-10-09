#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // customer_id is a raw column that must never reach the model -- Policy
  // drops it before learn_'s Model ever sees the frame.
  const DataFrame raw({
      {"customer_id", Slate(pluss::make_series({101, 102, 103, 104, 105, 106, 107, 108}, {8}, f64))},
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression(), policy);

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  std::cout << "before: trained = " << std::boolalpha << wf.trained() << "\n";
  wf.learn_(raw, data_options);
  std::cout << "after:  trained = " << wf.trained() << "\n";

  const pluss::Series pred = wf.infer(raw, data_options);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
