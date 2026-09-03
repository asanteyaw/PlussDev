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
  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  const lymph::selection::CompareResult ignored = builder.benchmark(x, y, cv);
  (void)ignored;

  // Both winners are real fitted models; compare their predictions on the same
  // probe rows.
  const lymph::models::Model top = builder.best();
  const lymph::models::Model bottom = builder.worst();
  // Probe rows in the overlap zone, where the top and bottom models can
  // genuinely disagree.
  const pluss::Series probe_income = pluss::make_series({2.6, 3.0, 3.4, 2.4}, {4}, f64);
  const pluss::Series probe_balance = pluss::make_series({2.4, 2.0, 1.7, 2.6}, {4}, f64);
  const DataFrame probe({{"income", Thread(probe_income)}, {"balance", Thread(probe_balance)}});

  const DataFrame compare({
      {"income", Thread(probe_income)},
      {"balance", Thread(probe_balance)},
      {"best_predicts", Thread(top.infer(probe))},
      {"worst_predicts", Thread(bottom.infer(probe))},
  });
  std::cout << compare;

  return 0;
}
