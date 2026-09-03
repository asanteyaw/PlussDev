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

  // The model was trained through the DataFrame + DFBatch contract, so
  // inference uses the same contract: a frame that may still carry the label
  // column, plus a batch that names the feature columns.
  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series defaulted = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame train_frame({
      {"income", Thread(income)},
      {"balance", Thread(balance)},
      {"defaulted", Thread(defaulted)},
  });

  lymph::options::DFBatch batch{};
  batch.feature_names = {"income", "balance"};
  batch.label_feature = "defaulted";

  const lymph::models::Model model =
      lymph::models::logistic_regression().learn(train_frame, batch);

  // A fresh frame. It carries a `defaulted` column too (unknown, filled with
  // zeros); infer ignores it because the batch only lists the two features.
  const DataFrame score_frame({
      {"income", Thread(pluss::make_series({1.2, 2.6, 4.0}, {3}, f64))},
      {"balance", Thread(pluss::make_series({0.4, 1.2, 2.2}, {3}, f64))},
      {"defaulted", Thread(pluss::make_series({0, 0, 0}, {3}, f64))},
  });

  const DataFrame result({
      {"income", score_frame.features("income")},
      {"balance", score_frame.features("balance")},
      {"predicted", Thread(model.infer(score_frame, batch))},
  });
  std::cout << result;

  return 0;
}
