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

  const DataFrame train_raw({
      {"customer_id", Slate(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(train_raw, data_options);

  // Every infer call re-applies Policy -- new incoming data can still carry
  // customer_id (or any other dropped/forbidden column), and it is stripped
  // the same way each time, exactly as it was at learn time.
  const DataFrame incoming({
      {"customer_id", Slate(pluss::make_series({9, 10, 11}, {3}, f64))},
      {"income", Slate(pluss::make_series({1.2, 4.4, 1.6}, {3}, f64))},
      {"balance", Slate(pluss::make_series({0.3, 2.3, 0.5}, {3}, f64))},
  });

  const pluss::Series pred = trained.infer(incoming, data_options);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
