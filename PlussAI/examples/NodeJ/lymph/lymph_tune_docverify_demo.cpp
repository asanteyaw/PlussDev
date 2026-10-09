#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using lymph::AssessOptions;
  using lymph::MetricKind;
  using lymph::TrainOptions;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"target", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late"};
  data_options.label_feature = "target";

  lymph::models::Builder builder = lymph::models::builder::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::decision_tree_classifier()},
  });

  lymph::selection::Suite suite = lymph::selection::spotcheck::suite(builder);
  suite.strategy("plain", lymph::TrainOptions{});
  suite.strategy("weighted", lymph::TrainOptions{
      .class_weight = lymph::ClassWeightMode::Balanced,
  });

  lymph::selection::CrossValOptions cv{};
  cv.folds = 4;
  cv.shuffle = true;
  cv.random_state = 20260825ULL;
  cv.deterministic = true;

  AssessOptions assess_options{};
  assess_options.metrics = {MetricKind::Accuracy, MetricKind::F1};

  const lymph::selection::spotcheck::Result suite_result =
      suite.run(
          frame,
          data_options,
          cv,
          assess_options,
          MetricKind::F1,
          lymph::TuneObjectivePolicy::Auto,
          {.fit_top = 1});
  std::cout << "== suite_compare_report ==\n";
  std::cout << "best=" << suite_result.report.best_name << "\n";
  std::cout << suite_result.report.board() << "\n";

  const std::vector<lymph::selection::GridAxis> axes = {
      {"regularization_strength", {1e-4, 1e-3, 1e-2}},
  };
  const lymph::selection::Search search = lymph::selection::search::grid(axes, cv);

  TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  const lymph::selection::search::Result search_result =
      search.run(
          [](const lymph::selection::GridPoint& point) {
            lymph::LinearFamilyOptions tuned{};
            tuned.regularization = lymph::RegularizationKind::L2;
            tuned.regularization_strength =
                point.at("regularization_strength");
            tuned.fit_intercept = true;
            return lymph::models::logistic_regression(tuned);
          },
          frame,
          data_options,
          train_options,
          assess_options,
          MetricKind::F1,
          lymph::TuneObjectivePolicy::Auto,
          {.fit_top = 1});

  std::cout << "== search_result ==\n";
  std::cout << "best=" << search_result.report.best_name << "\n";
  std::cout << search_result.report.board() << "\n";

  return 0;
}
