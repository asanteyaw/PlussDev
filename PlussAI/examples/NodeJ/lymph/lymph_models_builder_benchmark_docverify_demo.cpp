#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"x1", Slate(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"x2", Slate(pluss::make_series({1, 1, 2, 2, 8, 8, 9, 9}, {8}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression({.fit_intercept = true})},
      {"tree", lymph::models::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::knn_classifier({.k = 3})},
  });

  lymph::options::CrossVal cv{};
  cv.folds = 2;
  cv.shuffle = false;

  const lymph::selection::CompareResult ranked = builder.benchmark(
      x, y, cv, lymph::options::Train{}, lymph::options::Assess{}, lymph::MetricKind::Accuracy,
      lymph::TuneObjectivePolicy::Auto, lymph::options::Fit{.fit_top = 2});

  std::cout << "== benchmark ==\n";
  std::cout << ranked.report.board() << "\n\n";
  std::cout << "best_name = " << ranked.report.best_name << ", benchmarked = " << std::boolalpha
            << builder.benchmarked() << ", best_trained = " << builder.best().trained()
            << ", worst_trained = " << builder.worst().trained()
            << ", stored_logreg_trained = " << builder.get("logreg").trained() << "\n";

  return 0;
}
