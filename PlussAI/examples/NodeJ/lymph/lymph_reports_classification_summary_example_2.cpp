#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// reports::confusion_table(model, x, y) -- the count matrix, with inference
// done for you. model.reports().confusion_table(...) is the same call off a
// bound accessor.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"income", Slate(pluss::make_series({1.0, 1.2, 1.4, 1.1, 3.0, 3.2, 3.4, 3.1}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.5, 0.7, 0.6, 0.8, 2.5, 2.7, 2.6, 2.8}, {8}, f64))},
  });
  const Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const lymph::ConfusionTable table = lymph::reports::confusion_table(model, x, y);
  std::cout << "labels =\n" << table.labels << "\n";
  std::cout << "counts =\n" << table.counts << "\n";

  const lymph::ConfusionTable via_accessor = model.reports().confusion_table(x, y);
  std::cout << "accessor counts =\n" << via_accessor.counts << "\n";

  return 0;
}
