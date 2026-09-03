#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // One frame carries the features and the label side by side, the way a
  // real table loaded from disk usually does.
  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series defaulted = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame frame({
      {"income", Thread(income)},
      {"balance", Thread(balance)},
      {"defaulted", Thread(defaulted)},
  });

  // The batch spells out the data contract: which columns are features and
  // which single column is the label. `defaulted` can no longer be pulled in
  // as a feature by accident.
  lymph::options::DFBatch batch{};
  batch.feature_names = {"income", "balance"};
  batch.label_feature = "defaulted";

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(frame, batch);
  std::cout << "trained = " << std::boolalpha << model.trained() << "\n\n";

  // Inference takes the same frame and batch; only the feature columns are read.
  const DataFrame review({
      {"income", Thread(income)},
      {"balance", Thread(balance)},
      {"defaulted", Thread(defaulted)},
      {"predicted", Thread(model.infer(frame, batch))},
  });
  std::cout << review;

  return 0;
}
