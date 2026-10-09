#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// A Plan is a value. The chained setters -- folds, repeat, shuffle, seed,
// workers -- each return *this and mutate the held CrossValOptions, which
// options() reads back. One configured Plan can be run against many models.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"x", Slate(x)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  const lymph::selection::Plan plan =
      lymph::selection::cross().folds(5).repeat(2).shuffle(true).seed(20260901ULL).workers(1);

  const lymph::selection::CrossValOptions& o = plan.options();
  std::cout << "folds=" << o.folds << " repeats=" << o.repeats
            << " shuffle=" << std::boolalpha << o.shuffle
            << " seed=" << o.random_state
            << " deterministic=" << o.deterministic
            << " workers=" << o.max_parallel << "\n\n";

  // Same plan, two models.
  std::cout << "logreg mean accuracy = "
            << plan.score(lymph::models::logistic_regression(), frame, data_options,
                          lymph::MetricKind::Accuracy)
            << "\n";
  std::cout << "tree   mean accuracy = "
            << plan.score(lymph::models::tree::decision_tree_classifier({.max_depth = 2}),
                          frame, data_options, lymph::MetricKind::Accuracy)
            << "\n";

  return 0;
}
