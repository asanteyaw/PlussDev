#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// scores() takes one MetricKind and returns that metric's value on every fold.
// Here on a stratified plan with an imbalanced label, asking for F1 -- then
// reading the individual fold entries out of the returned Series.
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
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"score", Slate(score)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"score"};
  data_options.label_feature = "target";

  const lymph::selection::Plan plan =
      lymph::selection::stratified().folds(5).seed(20260901ULL);
  const lymph::models::Model model = lymph::models::logistic_regression();

  const pluss::Series f1_per_fold =
      plan.scores(model, frame, data_options, lymph::MetricKind::F1);
  const double f1_mean =
      plan.score(model, frame, data_options, lymph::MetricKind::F1);

  const std::vector<double> folds = f1_per_fold.host<double>();
  std::cout << "fold count = " << folds.size() << "\n";
  for (std::size_t i = 0; i < folds.size(); ++i) {
    std::cout << "  fold " << i << " f1 = " << folds[i] << "\n";
  }
  std::cout << "mean f1 = " << f1_mean << "\n";

  return 0;
}
