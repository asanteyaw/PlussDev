#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Plan::run(builder, ...) cross-validates every model a models::Builder holds
// under this plan and returns a selection::CompareResult -- the ranked board
// plus the fitted winner. It is Builder::benchmark reached through a Plan.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

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
  const DataFrame frame({{"income", Thread(income)}, {"balance", Thread(balance)},
                         {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  lymph::models::Builder candidates = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  const lymph::selection::Plan plan = lymph::selection::stratified().folds(4).seed(20260901ULL);

  const lymph::selection::CompareResult ranked =
      plan.run(candidates, frame, data_options, {}, {},
               lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "best = " << ranked.report.best_name
            << ", trials = " << ranked.report.trials.size()
            << ", winner trained = " << std::boolalpha << ranked.best().trained() << "\n\n";
  std::cout << ranked.report.board();

  return 0;
}
