#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// apply() is replayed on every Workflow verb, not just once at train time.
// A model trained through a Policy still gets that Policy applied to fresh
// inference data -- a dropped column on incoming rows is stripped again.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train_raw({
      {"customer_id", Slate(pluss::make_series({1, 2, 3, 4, 5, 6}, {6}, f64))},
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 3.6, 4.0, 4.4}, {6}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.6, 1.8, 2.1, 2.4}, {6}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(train_raw, data_options);

  // Incoming rows still carry customer_id -- infer applies Policy again.
  const DataFrame incoming({
      {"customer_id", Slate(pluss::make_series({7, 8, 9}, {3}, f64))},
      {"income", Slate(pluss::make_series({1.2, 4.2, 1.6}, {3}, f64))},
      {"balance", Slate(pluss::make_series({0.3, 2.2, 0.5}, {3}, f64))},
  });
  std::cout << "pred = " << trained.infer(incoming, data_options) << "\n";

  return 0;
}
