#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;
  using pluss::ai::nodej::AssessOptions;
  using pluss::ai::nodej::ClassReportOptions;
  using pluss::ai::nodej::ConfusionOptions;
  using pluss::ai::nodej::LinearFamilyOptions;
  using pluss::ai::nodej::MetricKind;
  using pluss::ai::nodej::RegularizationKind;
  using pluss::ai::nodej::TrainOptions;

  const pluss::Config f64 = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series(std::vector<double>{1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series(std::vector<double>{0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"target", Thread(pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"when", Thread(pluss::make_timeline({
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

  pluss::ai::nodej::DataFrameBatchOptions data_options{};
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

  const lymph::Model logistic = lymph::model::logistic_regression(logistic_options);

  lymph::ValidationOptions cv{};
  cv.folds = 4;
  cv.shuffle = true;
  cv.random_state = 20260825ULL;
  cv.deterministic = true;

  const lymph::ValidationPlan cross_plan =
      lymph::validate::cross(cv).folds(4).repeat(1).shuffle(true).seed(20260825ULL).workers(1);

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

  const lymph::HoldoutPlan holdout_plan =
      lymph::validate::holdout().test_size(0.25).shuffle(true).seed(20260825ULL);
  const auto holdout = holdout_plan.split(frame);
  std::cout << "== holdout_split ==\n";
  std::cout << "train_rows=" << holdout.train_set.ncases()
            << " test_rows=" << holdout.test_set.ncases() << "\n";
  const lymph::HoldoutPlan holdout_half_plan =
      lymph::validate::holdout().test_size(0.5).shuffle(false).seed(20260825ULL);
  const auto holdout_half = holdout_half_plan.split(frame);
  std::cout << "== holdout_split_half ==\n";
  std::cout << "train_rows=" << holdout_half.train_set.ncases()
            << " test_rows=" << holdout_half.test_set.ncases() << "\n";

  lymph::ValidationOptions time_options{};
  time_options.folds = 3;
  time_options.time_test_size = 2;
  time_options.time_gap = 0;
  time_options.shuffle = false;
  time_options.random_state = 20260825ULL;
  time_options.deterministic = true;
  const lymph::ValidationPlan time_plan =
      lymph::validate::time(time_options).gap(0).horizon(2).lookback(4);
  const auto last_block = time_plan.last(frame, "when");
  std::cout << "== time_plan_last ==\n";
  std::cout << "train_rows=" << last_block.train_set.ncases()
            << " test_rows=" << last_block.test_set.ncases() << "\n";
  std::cout << "train_last_when=" << last_block.train_set.features("when").tail(1)
            << " test_first_when=" << last_block.test_set.features("when").head(1) << "\n";
  const lymph::ValidationPlan time_gap_plan =
      lymph::validate::time(time_options).gap(1).horizon(2).lookback(3);
  const auto gap_block = time_gap_plan.last(frame, "when");
  std::cout << "== time_plan_gap_last ==\n";
  std::cout << "train_rows=" << gap_block.train_set.ncases()
            << " test_rows=" << gap_block.test_set.ncases() << "\n";
  std::cout << "train_last_when=" << gap_block.train_set.features("when").tail(1)
            << " test_first_when=" << gap_block.test_set.features("when").head(1) << "\n";

  lymph::Builder builder = lymph::builder::build({
      {"logreg", lymph::model::logistic_regression(logistic_options)},
      {"tree", lymph::model::decision_tree_classifier()},
  });

  std::cout << "== builder_report ==\n";
  std::cout << builder.report() << "\n";

  lymph::Suite suite = lymph::spotcheck::suite(builder);
  suite.strategy("plain", TrainOptions{});
  suite.strategy("weighted", TrainOptions{
      .class_weight = pluss::ai::nodej::ClassWeightMode::Balanced,
  });

  const lymph::spotcheck::Result suite_result =
      suite.run(frame, data_options, cv, assess_options, MetricKind::F1,
                lymph::TuneObjectivePolicy::Auto, {.fit_top = 1});
  std::cout << "== suite_compare_report ==\n";
  std::cout << "best=" << suite_result.report.best_name << "\n";
  std::cout << suite_result.report.board() << "\n";

  const std::vector<lymph::GridAxis> axes = {
      {"regularization_strength", {1e-4, 1e-3, 1e-2}},
  };
  const lymph::Search search = lymph::search::grid(axes, cv);
  const lymph::search::Result search_result =
      search.run(
          [](const lymph::GridPoint& point) {
            LinearFamilyOptions tuned{};
            tuned.regularization = RegularizationKind::L2;
            tuned.regularization_strength = point.at("regularization_strength");
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

  const Series predicted = logistic.learn(frame, data_options, train_options).infer(frame, data_options);
  std::cout << "== confusion_table ==\n";
  const lymph::ConfusionTable confusion = lymph::confusion_table(
      frame.features("target").as_series(),
      predicted,
      ConfusionOptions{
          .normalize = pluss::ai::nodej::ConfusionNormalize::ByTruth,
      });
  std::cout << "labels=" << confusion.labels << "\n";
  std::cout << "counts=" << confusion.counts << "\n";
  std::cout << "normalize=" << static_cast<int>(confusion.normalize) << "\n";

  std::cout << "== classification_summary ==\n";
  const lymph::ClassReport summary = lymph::classification_summary(
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
  const lymph::metrics::Threshold threshold = lymph::metrics::threshold(scores, frame.features("target").as_series());
  std::cout << "== threshold ==\n";
  std::cout << "best_f1=" << threshold.best(MetricKind::F1) << "\n";
  std::cout << threshold.curve().thresholds.slice(0, 0, 3) << "\n";

  return 0;
}
