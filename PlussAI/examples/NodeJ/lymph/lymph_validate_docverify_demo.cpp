#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::ai::nodej::AssessOptions;
  using pluss::ai::nodej::LinearFamilyOptions;
  using pluss::ai::nodej::MetricKind;
  using pluss::ai::nodej::RegularizationKind;
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

  const lymph::Model logistic = lymph::model::logistic_regression(logistic_options);

  lymph::ValidationOptions cross_options{};
  cross_options.folds = 4;
  cross_options.repeats = 1;
  cross_options.shuffle = true;
  cross_options.random_state = 20260825ULL;
  cross_options.deterministic = true;
  cross_options.max_parallel = 1;

  const lymph::ValidationPlan cross_plan = lymph::validate::cross(cross_options);
  std::cout << "== validation_options_cross ==\n";
  std::cout << "plan=" << static_cast<int>(cross_plan.options().plan)
            << " folds=" << cross_plan.options().folds
            << " shuffle=" << cross_plan.options().shuffle
            << " seed=" << cross_plan.options().random_state
            << " deterministic=" << cross_plan.options().deterministic
            << " workers=" << cross_plan.options().max_parallel << "\n";
  std::cout << "score(accuracy)="
            << cross_plan.score(
                   logistic,
                   frame,
                   data_options,
                   MetricKind::Accuracy,
                   train_options,
                   assess_options)
            << "\n";
  std::cout << "scores(f1)="
            << cross_plan.scores(
                   logistic,
                   frame,
                   data_options,
                   MetricKind::F1,
                   train_options,
                   assess_options)
            << "\n";

  const lymph::metrics::CrossValidationReport cross_report =
      cross_plan.run(logistic, frame, data_options, train_options, assess_options);
  std::cout << "== cross_validation_report ==\n";
  std::cout << cross_report.board() << "\n";

  const lymph::HoldoutPlan holdout_plan =
      lymph::validate::holdout().test_size(0.25).shuffle(true).seed(20260825ULL);
  const lymph::HoldoutSplit holdout = holdout_plan.split(frame);
  std::cout << "== holdout_split ==\n";
  std::cout << "train_rows=" << holdout.train_set.ncases()
            << " test_rows=" << holdout.test_set.ncases() << "\n";

  lymph::ValidationOptions time_options{};
  time_options.folds = 3;
  time_options.shuffle = false;
  time_options.time_gap = 0;
  time_options.time_test_size = 2;
  time_options.time_max_train_size = 4;
  time_options.random_state = 20260825ULL;
  time_options.deterministic = true;

  const lymph::ValidationPlan time_plan = lymph::validate::time(time_options);
  const lymph::TimeSeriesSplit last_block = time_plan.last(frame, "when");
  std::cout << "== time_plan_last ==\n";
  std::cout << "train_rows=" << last_block.train_set.ncases()
            << " test_rows=" << last_block.test_set.ncases() << "\n";
  std::cout << "train_last_when=" << last_block.train_set.features("when").tail(1)
            << " test_first_when=" << last_block.test_set.features("when").head(1)
            << "\n";

  return 0;
}
