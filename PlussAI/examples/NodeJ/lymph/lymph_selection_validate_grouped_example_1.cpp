#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// grouped(groups) never splits a group across the train/test boundary. Use it
// when rows share a leak-prone key -- the same customer, device, or subject
// appearing in several rows. Here four groups of five rows each, every group
// carrying both classes so each fold can be assessed.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Each group of 5 rows: 3 low (label 0) then 2 high (label 1).
  const pluss::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 5.0, 5.2, 1.1, 1.3, 1.5, 5.1, 5.3,
       1.0, 1.2, 1.4, 5.0, 5.2, 1.1, 1.3, 1.5, 5.1, 5.3}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1}, {20}, f64);
  const pluss::Series groups = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3}, {20}, f64);
  const DataFrame frame({{"x", Slate(x)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  // The group key is passed to grouped(...), not carried in the frame.
  const lymph::selection::Plan plan =
      lymph::selection::grouped(groups).folds(4).seed(20260901ULL);

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::logistic_regression(), frame, data_options);

  std::cout << "plan kind = " << static_cast<int>(plan.options().plan)
            << " (grouped), folds = " << report.report.folds.size() << "\n\n";
  std::cout << report.board();

  return 0;
}
