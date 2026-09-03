#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

namespace lymph = pluss::ai::lymph;
using pluss::ScalarType;
using pluss::table::DataFrame;
using pluss::table::Thread;

const pluss::Config kF64 =
    pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

DataFrame features() {
  return DataFrame({
      {"x1", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, kF64))},
      {"x2", Thread(pluss::make_series({2, 2, 3, 3, 7, 7, 8, 8}, {8}, kF64))},
  });
}

pluss::Series labels() { return pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, kF64); }

DataFrame labelled() {
  return DataFrame({
      {"x1", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, kF64))},
      {"x2", Thread(pluss::make_series({2, 2, 3, 3, 7, 7, 8, 8}, {8}, kF64))},
      {"label", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, kF64))},
  });
}

lymph::models::Builder candidates() {
  return lymph::models::build({
      {"logreg", lymph::models::linear::logistic_regression({.fit_intercept = true})},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::knn_classifier({.k = 3})},
  });
}

lymph::options::DFBatch frame_options() {
  lymph::options::DFBatch options{};
  options.feature_names = {"x1", "x2"};
  options.label_feature = "label";
  return options;
}

void print_names(const char *label, const std::vector<std::string> &names) {
  std::cout << label << '=';
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i != 0)
      std::cout << ',';
    std::cout << names[i];
  }
  std::cout << '\n';
}

} // namespace

int main() {
  const DataFrame x = features();
  const pluss::Series y = labels();
  const DataFrame frame = labelled();
  const lymph::options::DFBatch options = frame_options();

  // INJECT, HAS, GET, EJECT, NAMES, LIST, AND REPORT
  auto registry = lymph::models::build();
  registry.inject("logreg", lymph::models::linear::logistic_regression())
      .inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}));
  std::cout << "inject_chain size=" << registry.size() << '\n';

  const auto map_registry = candidates();
  std::cout << "inject_map size=" << map_registry.size() << '\n';

  bool duplicate_rejected = false;
  try {
    registry.inject("tree", lymph::models::tree::decision_tree_classifier());
  } catch (const std::exception &) {
    duplicate_rejected = true;
  }
  std::cout << "inject_duplicate_rejected=" << std::boolalpha << duplicate_rejected << '\n';

  std::cout << "has_present=" << registry.has("tree") << '\n';
  std::cout << "has_missing=" << registry.has("missing") << '\n';
  const auto removed = registry.eject("tree");
  std::cout << "has_after_eject=" << registry.has("tree")
            << " removed_trained=" << removed.trained() << '\n';

  const auto &read_only = static_cast<const lymph::models::Builder &>(map_registry).get("logreg");
  std::cout << "get_const_trained=" << read_only.trained() << '\n';
  auto mutable_registry = candidates();
  mutable_registry.get("tree").standard_scaler();
  std::cout << "get_mutable_has_tree=" << mutable_registry.has("tree") << '\n';
  bool missing_get_rejected = false;
  try {
    [[maybe_unused]] const auto &missing = mutable_registry.get("missing");
  } catch (const std::exception &) {
    missing_get_rejected = true;
  }
  std::cout << "get_missing_rejected=" << missing_get_rejected << '\n';

  print_names("names_all", map_registry.names());
  print_names("list_one", map_registry.list(1));
  print_names("list_all", map_registry.list());
  std::cout << "report_rows=" << map_registry.report().ncases() << '\n';
  std::cout << "report_preview_rows=" << map_registry.report(2).ncases() << '\n';
  std::cout << "report_empty_rows=" << lymph::models::build().report().ncases() << '\n';

  // BENCHMARK, BENCHMARKED, BEST, AND WORST
  lymph::options::CrossVal cv{};
  cv.folds = 2;
  cv.shuffle = false;

  auto benchmark_registry = candidates();
  std::cout << "benchmarked_initial=" << benchmark_registry.benchmarked() << '\n';
  const auto xy_result = benchmark_registry.benchmark(x, y, cv);
  std::cout << "benchmark_xy rows=" << xy_result.report.board().ncases()
            << " remembered=" << benchmark_registry.benchmarked() << '\n';

  const auto frame_result = benchmark_registry.benchmark(frame, options, cv);
  std::cout << "benchmark_frame rows=" << frame_result.report.board().ncases() << '\n';

  const auto accuracy_result =
      benchmark_registry.benchmark(x, y, cv, {}, {}, lymph::MetricKind::Accuracy);
  std::cout << "benchmark_objective best=" << accuracy_result.report.best_name << '\n';

  std::cout << "best_trained=" << benchmark_registry.best().trained() << '\n';
  std::cout << "worst_trained=" << benchmark_registry.worst().trained() << '\n';
  bool best_before_benchmark_rejected = false;
  try {
    [[maybe_unused]] const auto best = candidates().best();
  } catch (const std::exception &) {
    best_before_benchmark_rejected = true;
  }
  std::cout << "best_before_benchmark_rejected=" << best_before_benchmark_rejected << '\n';

  auto replacement_registry = candidates();
  [[maybe_unused]] const auto old_tree = replacement_registry.eject("tree");
  replacement_registry.inject("tree", lymph::models::tree::decision_tree_classifier());
  std::cout << "eject_replace_has_tree=" << replacement_registry.has("tree") << '\n';
  bool optional_removed = false;
  if (replacement_registry.has("optional_model")) {
    [[maybe_unused]] const auto optional = replacement_registry.eject("optional_model");
    optional_removed = true;
  }
  std::cout << "eject_optional_removed=" << optional_removed << '\n';

  std::cout << "names_empty=" << lymph::models::build().names().empty() << '\n';
  print_names("names_after_eject", registry.names());
  print_names("list_negative", map_registry.list(-3));

  std::cout << "benchmarked_after_second_run=" << benchmark_registry.benchmarked() << '\n';
  std::cout << "best_cases=" << benchmark_registry.best().infer(x).size().item<std::int64_t>()
            << '\n';
  std::cout << "worst_cases=" << benchmark_registry.worst().infer(x).size().item<std::int64_t>()
            << '\n';

  auto single_inject_registry = lymph::models::build();
  single_inject_registry.inject("knn", lymph::models::knn_classifier({.k = 3}));
  std::cout << "inject_single size=" << single_inject_registry.size() << '\n';
  bool worst_before_benchmark_rejected = false;
  try {
    [[maybe_unused]] const auto worst = candidates().worst();
  } catch (const std::exception &) {
    worst_before_benchmark_rejected = true;
  }
  std::cout << "worst_before_benchmark_rejected=" << worst_before_benchmark_rejected << '\n';

  return 0;
}
