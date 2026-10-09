#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// stratified() keeps each fold's class balance close to the whole set's. On a
// skewed label (here 15 negatives, 5 positives) a plain split can hand a fold
// zero positives; stratified folds cannot.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series score = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8,
       3.0, 3.2, 3.4, 3.6, 3.8, 6.0, 6.4, 6.8, 7.2, 7.6}, {20}, f64);
  //                        five positives at the top only
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"score", Slate(score)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"score"};
  data_options.label_feature = "target";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  const lymph::selection::Plan plan =
      lymph::selection::stratified().folds(5).seed(20260901ULL);

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::logistic_regression(), frame, data_options, {}, assess);

  std::cout << "plan kind = " << static_cast<int>(plan.options().plan)
            << " (stratified), folds = " << report.report.folds.size() << "\n\n";
  std::cout << report.board();

  return 0;
}
