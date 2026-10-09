#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  using lymph::AssessOptions;
  using lymph::ClassReportOptions;
  using lymph::ConfusionOptions;
  using lymph::LinearFamilyOptions;
  using lymph::MetricKind;
  using lymph::RegularizationKind;
  using lymph::TrainOptions;

  const pluss::Config f64 = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Slate(pluss::make_series(std::vector<double>{1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Slate(pluss::make_series(std::vector<double>{0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Slate(pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"target", Slate(pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"when", Slate(pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z",
          "2026-01-08T00:00:00Z",
      }))},
  });

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late"};
  data_options.label_feature = "target";

  TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  AssessOptions assess_options{};
  assess_options.metrics = {MetricKind::Accuracy, MetricKind::F1};

  LinearFamilyOptions logistic_options{};
  logistic_options.regularization = RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  const lymph::models::Model logistic = lymph::models::logistic_regression(logistic_options);

  lymph::selection::CrossValOptions cv{};
  cv.folds = 4;
  cv.repeats = 1;
  cv.shuffle = true;
  cv.random_state = 20260825ULL;
  cv.deterministic = true;
  cv.max_parallel = 1;

  const lymph::selection::Plan cross_plan = lymph::selection::cross(cv);

  std::cout << "== validation_options_cross ==\n";
  std::cout << "plan=" << static_cast<int>(cross_plan.options().plan)
            << " folds=" << cross_plan.options().folds
            << " shuffle=" << cross_plan.options().shuffle
            << " seed=" << cross_plan.options().random_state
            << " deterministic=" << cross_plan.options().deterministic
            << " workers=" << cross_plan.options().max_parallel << "\n";
  std::cout << "score(accuracy)="
            << cross_plan.score(logistic, frame, data_options, MetricKind::Accuracy, train_options, assess_options)
            << "\n";
  std::cout << "scores(f1)="
            << cross_plan.scores(logistic, frame, data_options, MetricKind::F1, train_options, assess_options)
            << "\n";
  const lymph::metrics::CrossValidationReport cross_report =
      cross_plan.run(logistic, frame, data_options, train_options, assess_options);
  std::cout << "== cross_validation_report ==\n";
  std::cout << cross_report.board() << "\n";

  const lymph::selection::HoldoutPlan holdout_plan =
      lymph::selection::holdout().test_size(0.25).shuffle(true).seed(20260825ULL);
  const lymph::prep::HoldoutSplit holdout = holdout_plan.split(frame);
  std::cout << "== holdout_split ==\n";
  std::cout << "train_rows=" << holdout.train_set.ncases()
            << " test_rows=" << holdout.test_set.ncases() << "\n";
  const lymph::selection::HoldoutPlan holdout_half_plan =
      lymph::selection::holdout().test_size(0.5).shuffle(false).seed(20260825ULL);
  const lymph::prep::HoldoutSplit holdout_half = holdout_half_plan.split(frame);
  std::cout << "== holdout_split_half ==\n";
  std::cout << "train_rows=" << holdout_half.train_set.ncases()
            << " test_rows=" << holdout_half.test_set.ncases() << "\n";

  lymph::selection::CrossValOptions time_options{};
  time_options.folds = 3;
  time_options.time_test_size = 2;
  time_options.time_gap = 0;
  time_options.time_max_train_size = 4;
  time_options.shuffle = false;
  time_options.random_state = 20260825ULL;
  time_options.deterministic = true;
  const lymph::selection::Plan time_plan = lymph::selection::time(time_options);
  const lymph::prep::TimeSeriesSplit last_block = time_plan.last(frame, "when");
  std::cout << "== time_plan_last ==\n";
  std::cout << "train_rows=" << last_block.train_set.ncases()
            << " test_rows=" << last_block.test_set.ncases() << "\n";
  std::cout << "train_last_when=" << last_block.train_set.features("when").tail(1)
            << " test_first_when=" << last_block.test_set.features("when").head(1) << "\n";

  lymph::selection::CrossValOptions time_gap_options = time_options;
  time_gap_options.time_gap = 1;
  time_gap_options.time_max_train_size = 3;
  const lymph::selection::Plan time_gap_plan = lymph::selection::time(time_gap_options);
  const lymph::prep::TimeSeriesSplit gap_block = time_gap_plan.last(frame, "when");
  std::cout << "== time_plan_gap_last ==\n";
  std::cout << "train_rows=" << gap_block.train_set.ncases()
            << " test_rows=" << gap_block.test_set.ncases() << "\n";
  std::cout << "train_last_when=" << gap_block.train_set.features("when").tail(1)
            << " test_first_when=" << gap_block.test_set.features("when").head(1) << "\n";

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression(logistic_options)},
      {"tree", lymph::models::tree::decision_tree_classifier()},
  });

  std::cout << "== builder_report ==\n";
  std::cout << builder.report() << "\n";

  const lymph::selection::spotcheck::Result suite_result =
      lymph::selection::suite(builder)
          .strategy("plain", TrainOptions{})
          .strategy("weighted", TrainOptions{.class_weight = lymph::ClassWeightMode::Balanced})
          .run(frame, data_options, cv, assess_options, MetricKind::F1,
               lymph::TuneObjectivePolicy::Maximize, lymph::selection::FitOptions{.fit_top = 1});
  std::cout << "== suite_compare_report ==\n";
  std::cout << "best=" << suite_result.report.best_name << "\n";
  std::cout << suite_result.report.board() << "\n";

  const std::vector<lymph::selection::GridAxis> axes = {
      {"regularization_strength", {1e-4, 1e-3, 1e-2}},
  };
  const lymph::selection::Search search = lymph::selection::grid(axes, cv);
  const lymph::selection::search::Result search_result =
      search.run(
          [](const lymph::selection::GridPoint& point) {
            LinearFamilyOptions tuned{};
            tuned.regularization = RegularizationKind::L2;
            tuned.regularization_strength = point.at("regularization_strength");
            tuned.fit_intercept = true;
            return lymph::models::logistic_regression(tuned);
          },
          frame,
          data_options,
          train_options,
          assess_options,
          MetricKind::F1,
          lymph::TuneObjectivePolicy::Maximize,
          lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "== search_result ==\n";
  std::cout << "best=" << search_result.report.best_name << "\n";
  std::cout << search_result.report.board() << "\n";

  const Series predicted = logistic.learn(frame, data_options, train_options).infer(frame, data_options);
  std::cout << "== confusion_table ==\n";
  const lymph::ConfusionTable confusion = lymph::metrics::classification::confusion_table(
      frame.features("target").as_series(),
      predicted,
      ConfusionOptions{
          .normalize = lymph::ConfusionNormalize::ByTruth,
      });
  std::cout << "labels=" << confusion.labels << "\n";
  std::cout << "counts=" << confusion.counts << "\n";
  std::cout << "normalize=" << static_cast<int>(confusion.normalize) << "\n";

  std::cout << "== classification_summary ==\n";
  const lymph::ClassReport summary = lymph::metrics::classification::summary(
      frame.features("target").as_series(),
      predicted,
      ClassReportOptions{
          .zero_division = 0.0,
      });
  std::cout << "labels=" << summary.labels << "\n";
  std::cout << "precision=" << summary.precision << "\n";
  std::cout << "recall=" << summary.recall << "\n";
  std::cout << "f1=" << summary.f1 << "\n";
  std::cout << "accuracy=" << summary.accuracy << "\n";
  std::cout << "macro_average=" << summary.macro_average << "\n";

  const Series scores = logistic.learn(frame, data_options, train_options).infer_scores(frame, data_options);
  const lymph::metrics::classification::Threshold threshold =
      lymph::metrics::classification::threshold(scores, frame.features("target").as_series());
  std::cout << "== threshold ==\n";
  std::cout << "best_f1=" << threshold.best(MetricKind::F1) << "\n";
  std::cout << threshold.curve().thresholds.slice(0, 0, 3) << "\n";

  return 0;
}
