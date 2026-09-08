#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// reports::classification_summary(model, frame, data_options) -- inference
// plus scoring in one call. It is exactly metrics::classification::summary
// with model.infer(...) folded in, so the numbers match.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.2, 1.4, 1.1, 3.0, 3.2, 3.4, 3.1}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.5, 0.7, 0.6, 0.8, 2.5, 2.7, 2.6, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(frame, data_options);

  // The facade call:
  const lymph::ClassReport via_reports =
      lymph::reports::classification_summary(model, frame, data_options);

  // The equivalent by hand:
  const DataFrame x({{"income", frame.features("income")}, {"balance", frame.features("balance")}});
  const Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const lymph::ClassReport by_hand =
      lymph::metrics::classification::summary(y, model.infer(x));

  std::cout << "reports accuracy = " << via_reports.accuracy.item<double>() << "\n";
  std::cout << "by-hand accuracy = " << by_hand.accuracy.item<double>() << "\n";
  std::cout << "reports f1        =\n" << via_reports.f1 << "\n";
  std::cout << "reports mcc      = " << via_reports.mcc.item<double>() << "\n";

  return 0;
}
