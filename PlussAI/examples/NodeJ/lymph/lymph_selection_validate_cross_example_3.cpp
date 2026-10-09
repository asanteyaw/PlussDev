#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// repeat(n) runs the whole k-fold split n times with different shuffles, then
// pools all n*k folds. It tightens the estimate on a small dataset where a
// single split is noisy.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Slate(income)}, {"balance", Slate(balance)},
                         {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  const lymph::selection::Plan plan =
      lymph::selection::cross().folds(4).repeat(3).seed(20260901ULL);

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::logistic_regression(), frame, data_options);

  std::cout << "folds = " << report.report.folds.size()
            << " (4 folds x 3 repeats)\n\n";
  std::cout << report.board();

  return 0;
}
