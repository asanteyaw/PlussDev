#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::ai::nodej::AssessOptions;
  using pluss::ai::nodej::MetricKind;
  using pluss::ai::nodej::TrainOptions;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"target", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late"};
  data_options.label_feature = "target";

  lymph::Builder builder = lymph::builder::build({
      {"logreg", lymph::model::logistic_regression()},
      {"tree", lymph::model::decision_tree_classifier()},
  });

  lymph::Suite suite = lymph::spotcheck::suite(builder);
  suite.strategy("plain", lymph::TrainOptions{});
  suite.strategy("weighted", lymph::TrainOptions{
      .class_weight = lymph::ClassWeightMode::Balanced,
  });

  lymph::ValidationOptions cv{};
  cv.folds = 4;
  cv.shuffle = true;
  cv.random_state = 20260825ULL;
  cv.deterministic = true;

  AssessOptions assess_options{};
  assess_options.metrics = {MetricKind::Accuracy, MetricKind::F1};

  const lymph::spotcheck::Result suite_result =
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

  const std::vector<lymph::GridAxis> axes = {
      {"regularization_strength", {1e-4, 1e-3, 1e-2}},
  };
  const lymph::Search search = lymph::search::grid(axes, cv);

  TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  const lymph::search::Result search_result =
      search.run(
          [](const lymph::GridPoint& point) {
            lymph::LinearFamilyOptions tuned{};
            tuned.regularization = lymph::RegularizationKind::L2;
            tuned.regularization_strength =
                point.at("regularization_strength");
            tuned.fit_intercept = true;
            return lymph::model::logistic_regression(tuned);
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
