#include <pluss/ml.h>
#include <pluss/mic.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
  using DataFrame = pluss::table::DataFrame;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::Slate;
  using namespace pluss::ai::nodej;

  //========== setup: paths, controls, and output destinations ========
  // This section defines runtime paths and row budget.
  fs::path data_path = fs::path(PLUSSMIC_SOURCE_DIR) / "data" / "hour.csv";
  fs::path out_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "outputs" / "nodej_end_to_end_hour_workflow_showcase";
  fs::path model_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "NodeJ" / "trained_models";
  std::int64_t ncases = 14000;
  if (argc > 1 && argv[1] != nullptr && std::string(argv[1]).size() > 0) data_path = fs::path(argv[1]);
  if (argc > 2 && argv[2] != nullptr && std::string(argv[2]).size() > 0) out_dir = fs::path(argv[2]);
  if (argc > 3 && argv[3] != nullptr && std::string(argv[3]).size() > 0) ncases = std::stoll(argv[3]);
  fs::create_directories(out_dir);
  fs::create_directories(model_dir);

  //========== data loading ========
  // This section reads the PlussMic bike-sharing hourly data.
  DataFrame raw;
  try {
    raw = DataFrame::load_csv(data_path.string(), {{"ncases", ncases}});
  } catch (const std::exception& ex) {
    std::cerr << "load_csv failed: " << ex.what() << "\n";
    return 1;
  }

  //========== schema normalization ========
  // This section gives consistent names and explicit core column dtypes.
  DataFrame frame = raw.rename({
      {"dteday", "date"},
      {"holiday", "is_holiday"},
      {"workingday", "is_workingday"},
      {"weathersit", "weather_condition"},
      {"hum", "humidity"},
      {"mnth", "month"},
      {"cnt", "total_count"},
      {"hr", "hour"},
      {"yr", "year"},
  });

  // Selection belongs on features(); cast stays a pure transform surface.
  frame.set("date") = frame.features("date").cast().timeline();
  frame = frame.astype({
      {"season", ScalarType::Float64},
      {"year", ScalarType::Float64},
      {"month", ScalarType::Float64},
      {"hour", ScalarType::Float64},
      {"is_holiday", ScalarType::Float64},
      {"weekday", ScalarType::Float64},
      {"is_workingday", ScalarType::Float64},
      {"weather_condition", ScalarType::Float64},
      {"temp", ScalarType::Float64},
      {"atemp", ScalarType::Float64},
      {"humidity", ScalarType::Float64},
      {"windspeed", ScalarType::Float64},
      {"casual", ScalarType::Float64},
      {"registered", ScalarType::Float64},
      {"total_count", ScalarType::Float64},
  });

  //========== feature engineering ========
  // This section creates interaction and nonlinear terms directly on DataFrame columns.
  const Series s_temp = frame.features("temp").as_series();
  const Series s_atemp = frame.features("atemp").as_series();
  const Series s_humidity = frame.features("humidity").as_series();
  const Series s_windspeed = frame.features("windspeed").as_series();
  const Series s_hour = frame.features("hour").as_series();
  const Series s_month = frame.features("month").as_series();
  const Series s_weather = frame.features("weather_condition").as_series();

  frame = frame.assign(std::vector<std::pair<std::string, Slate>>{
      {"temp_sq", Slate(s_temp.square())},
      {"atemp_sq", Slate(s_atemp.square())},
      {"humidity_sq", Slate(s_humidity.square())},
      {"windspeed_sq", Slate(s_windspeed.square())},
      {"temp_humidity_interaction", Slate(s_temp.mul(s_humidity))},
      {"temp_windspeed_interaction", Slate(s_temp.mul(s_windspeed))},
      {"atemp_humidity_interaction", Slate(s_atemp.mul(s_humidity))},
      {"atemp_windspeed_interaction", Slate(s_atemp.mul(s_windspeed))},
      {"temp_minus_atemp", Slate(s_temp.sub(s_atemp))},
      {"hour_centered", Slate(s_hour.sub(12.0))},
      {"hour_centered_sq", Slate(s_hour.sub(12.0).square())},
      {"month_centered", Slate(s_month.sub(6.5))},
      {"month_centered_sq", Slate(s_month.sub(6.5).square())},
      {"weather_sq", Slate(s_weather.square())},
  });

  //========== missingness diagnostics ========
  // This section computes missingness rates before model preprocessing.
  const DataFrame missing_binary = frame.isnull({
      "temp", "atemp", "humidity", "windspeed", "total_count",
      "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
  });
  // Axis::Case reduces down each column across all rows, producing one missingness rate per
  // feature; the result is a Feature-labeled Slate, so its own labels are the original feature
  // names. Those labels are surfaced both as an explicit "feature" column (so they show up in
  // the CSV, which is saved with the library's default add_cases=false) and as the summary
  // frame's own case labels.
  const Slate missing_rate_slate = missing_binary.mean(pluss::table::Axis::Case);
  const DataFrame missing_rate = DataFrame(
      {
          {"feature", missing_rate_slate.label().values()},
          {"missing_rate", missing_rate_slate},
      },
      missing_rate_slate.label().values());
  missing_rate.save_csv((out_dir / "missing_rate.csv").string());

  //========== exploratory data analysis (EDA) ========
  // This section saves descriptive statistics and correlations.
  const DataFrame numeric_block = frame.features({
      "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition",
      "temp", "atemp", "humidity", "windspeed", "casual", "registered", "total_count",
      "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
      "temp_humidity_interaction", "temp_windspeed_interaction", "atemp_humidity_interaction", "atemp_windspeed_interaction",
      "temp_minus_atemp", "hour_centered", "hour_centered_sq", "month_centered", "month_centered_sq", "weather_sq",
  });
  numeric_block.describe(false).save_csv((out_dir / "eda_describe.csv").string());
  numeric_block.corr(false).save_csv((out_dir / "eda_corr.csv").string());

  // This section generates compact EDA figures.
  frame.plot()
      .hist("total_count", 80)
      .headline("Bike demand distribution")
      .save_pdf((out_dir / "eda_hist_total_count.pdf").string());
  frame.plot()
      .scatter("temp", "total_count")
      .headline("Demand vs temperature")
      .save_pdf((out_dir / "eda_scatter_temp_target.pdf").string());
  frame.plot()
      .scatter("humidity", "total_count")
      .headline("Demand vs humidity")
      .save_pdf((out_dir / "eda_scatter_humidity_target.pdf").string());
  frame.plot()
      .scatter("windspeed", "total_count")
      .headline("Demand vs windspeed")
      .save_pdf((out_dir / "eda_scatter_windspeed_target.pdf").string());
  frame.plot()
      .scatter("hour", "total_count")
      .headline("Demand vs hour")
      .save_pdf((out_dir / "eda_scatter_hour_target.pdf").string());

  frame.group({.dropnull = true})
      .by({"hour"})
      .agg({
          {"mean_total_count", "total_count", DataFrame::GroupOp::Mean},
          {"count", "total_count", DataFrame::GroupOp::Count},
      })
      .save_csv((out_dir / "eda_hour_mean_total_count.csv").string());

  frame.group({.dropnull = true})
      .by({"weather_condition"})
      .agg({
          {"mean_total_count", "total_count", DataFrame::GroupOp::Mean},
          {"count", "total_count", DataFrame::GroupOp::Count},
      })
      .save_csv((out_dir / "eda_weather_mean_total_count.csv").string());

  frame.group({.dropnull = true})
      .by({"month"})
      .agg({
          {"mean_total_count", "total_count", DataFrame::GroupOp::Mean},
          {"count", "total_count", DataFrame::GroupOp::Count},
      })
      .save_csv((out_dir / "eda_month_mean_total_count.csv").string());

  frame.group({.dropnull = true})
      .by({"weekday"})
      .agg({
          {"mean_total_count", "total_count", DataFrame::GroupOp::Mean},
          {"count", "total_count", DataFrame::GroupOp::Count},
      })
      .save_csv((out_dir / "eda_weekday_mean_total_count.csv").string());

  //========== holdout split ========
  // This section performs deterministic holdout split using core split API.
  SplitOptions holdout_options{};
  holdout_options.holdout_fraction = 0.2;
  holdout_options.shuffle = true;
  holdout_options.random.seed = 42;
  holdout_options.random.deterministic = true;

  StatusOr<IndexSplit> holdout_or = holdout_split(frame.ncases(), holdout_options);
  if (!holdout_or.ok()) {
    std::cerr << "holdout_split failed: " << holdout_or.status().message << "\n";
    return 1;
  }
  const IndexSplit holdout = holdout_or.move_value();

  const std::vector<std::int64_t> holdout_train_idx = holdout.train_indices.to(pluss::pCPU, ScalarType::Int64)
                                                           .reshape({holdout.train_indices.numel()})
                                                           .host<std::int64_t>();
  const std::vector<std::int64_t> holdout_test_idx = holdout.test_indices.to(pluss::pCPU, ScalarType::Int64)
                                                         .reshape({holdout.test_indices.numel()})
                                                         .host<std::int64_t>();

  const DataFrame train_frame_full = frame.take(holdout_train_idx);
  const DataFrame test_frame_full = frame.take(holdout_test_idx);

  const DataFrame train_x = train_frame_full.features({
      "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition",
      "temp", "atemp", "humidity", "windspeed", "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
      "temp_humidity_interaction", "temp_windspeed_interaction", "atemp_humidity_interaction", "atemp_windspeed_interaction",
      "temp_minus_atemp", "hour_centered", "hour_centered_sq", "month_centered", "month_centered_sq", "weather_sq",
  });
  const DataFrame test_x = test_frame_full.features({
      "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition",
      "temp", "atemp", "humidity", "windspeed", "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
      "temp_humidity_interaction", "temp_windspeed_interaction", "atemp_humidity_interaction", "atemp_windspeed_interaction",
      "temp_minus_atemp", "hour_centered", "hour_centered_sq", "month_centered", "month_centered_sq", "weather_sq",
  });
  const Series train_y = train_frame_full.features("total_count").as_series();
  const Series test_y = test_frame_full.features("total_count").as_series();

  std::vector<std::string> model_feature_names = {
      "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition",
      "temp", "atemp", "humidity", "windspeed", "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
      "temp_humidity_interaction", "temp_windspeed_interaction", "atemp_humidity_interaction", "atemp_windspeed_interaction",
      "temp_minus_atemp", "hour_centered", "hour_centered_sq", "month_centered", "month_centered_sq", "weather_sq",
  };

  //========== standalone core preprocessors ========
  // This section demonstrates raw core prep classes on FeatureBatch.
  DataFrameBatchOptions x_batch_options{};
  x_batch_options.feature_names = model_feature_names;
  x_batch_options.layout = ObservationLayout::Rows;
  x_batch_options.slate_policy = DataSlatePolicy::CastTemporalToSeries;
  x_batch_options.note_policy = DataNotePolicy::Reject;

  StatusOr<FeatureBatch> train_x_batch_or = feature_batch_from_dataframe(train_x, x_batch_options);
  if (!train_x_batch_or.ok()) {
    std::cerr << "feature_batch_from_dataframe(train_x) failed: " << train_x_batch_or.status().message << "\n";
    return 1;
  }
  FeatureBatch train_x_batch = train_x_batch_or.move_value();

  StatusOr<FeatureBatch> test_x_batch_or = feature_batch_from_dataframe(test_x, x_batch_options);
  if (!test_x_batch_or.ok()) {
    std::cerr << "feature_batch_from_dataframe(test_x) failed: " << test_x_batch_or.status().message << "\n";
    return 1;
  }
  FeatureBatch test_x_batch = test_x_batch_or.move_value();

  prep::SimpleImputer simple_imputer(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Median});
  StatusOr<Series> train_simple_imputed_or = simple_imputer.learn_map(train_x_batch);
  if (!train_simple_imputed_or.ok()) {
    std::cerr << "SimpleImputer.learn_map(train) failed: " << train_simple_imputed_or.status().message << "\n";
    return 1;
  }
  Series train_simple_imputed = train_simple_imputed_or.move_value();

  StatusOr<Series> test_simple_imputed_or = simple_imputer.map(test_x_batch);
  if (!test_simple_imputed_or.ok()) {
    std::cerr << "SimpleImputer.map(test) failed: " << test_simple_imputed_or.status().message << "\n";
    return 1;
  }
  Series test_simple_imputed = test_simple_imputed_or.move_value();

  prep::NeighborImputer knn_imputer(prep::NeighborImputerOptions{
      .neighbors = 7,
      .weighting = prep::NeighborImputerWeighting::Distance,
      .distance_floor = 1e-12,
      .empty_feature_fallback = 0.0,
  });
  StatusOr<Series> train_knn_imputed_or = knn_imputer.learn_map(train_x_batch);
  if (!train_knn_imputed_or.ok()) {
    std::cerr << "NeighborImputer.learn_map(train) failed: " << train_knn_imputed_or.status().message << "\n";
    return 1;
  }
  Series train_knn_imputed = train_knn_imputed_or.move_value();

  StatusOr<Series> test_knn_imputed_or = knn_imputer.map(test_x_batch);
  if (!test_knn_imputed_or.ok()) {
    std::cerr << "NeighborImputer.map(test) failed: " << test_knn_imputed_or.status().message << "\n";
    return 1;
  }
  Series test_knn_imputed = test_knn_imputed_or.move_value();

  prep::IterativeImputer iterative_imputer(prep::IterativeImputerOptions{
      .rounds = 6,
      .ridge = 1e-5,
      .empty_feature_fallback = 0.0,
  });
  StatusOr<Series> train_iter_imputed_or = iterative_imputer.learn_map(train_x_batch);
  if (!train_iter_imputed_or.ok()) {
    std::cerr << "IterativeImputer.learn_map(train) failed: " << train_iter_imputed_or.status().message << "\n";
    return 1;
  }
  Series train_iter_imputed = train_iter_imputed_or.move_value();

  StatusOr<Series> test_iter_imputed_or = iterative_imputer.map(test_x_batch);
  if (!test_iter_imputed_or.ok()) {
    std::cerr << "IterativeImputer.map(test) failed: " << test_iter_imputed_or.status().message << "\n";
    return 1;
  }
  Series test_iter_imputed = test_iter_imputed_or.move_value();

  prep::StandardScaler standard_scaler(prep::StandardScalerOptions{
      .with_centering = true,
      .with_scaling = true,
      .variance_floor = 1e-12,
  });
  FeatureBatch train_simple_batch{.x = train_simple_imputed, .layout = ObservationLayout::Rows};
  FeatureBatch test_simple_batch{.x = test_simple_imputed, .layout = ObservationLayout::Rows};

  StatusOr<Series> train_standard_scaled_or = standard_scaler.learn_map(train_simple_batch);
  if (!train_standard_scaled_or.ok()) {
    std::cerr << "StandardScaler.learn_map(train) failed: " << train_standard_scaled_or.status().message << "\n";
    return 1;
  }
  Series train_standard_scaled = train_standard_scaled_or.move_value();

  StatusOr<Series> test_standard_scaled_or = standard_scaler.map(test_simple_batch);
  if (!test_standard_scaled_or.ok()) {
    std::cerr << "StandardScaler.map(test) failed: " << test_standard_scaled_or.status().message << "\n";
    return 1;
  }
  Series test_standard_scaled = test_standard_scaled_or.move_value();

  prep::RobustScaler robust_scaler(prep::RobustScalerOptions{
      .with_centering = true,
      .with_scaling = true,
      .lower_quantile = 0.15,
      .upper_quantile = 0.85,
      .range_floor = 1e-9,
  });
  StatusOr<Series> train_robust_scaled_or = robust_scaler.learn_map(train_simple_batch);
  if (!train_robust_scaled_or.ok()) {
    std::cerr << "RobustScaler.learn_map(train) failed: " << train_robust_scaled_or.status().message << "\n";
    return 1;
  }
  Series train_robust_scaled = train_robust_scaled_or.move_value();

  StatusOr<Series> test_robust_scaled_or = robust_scaler.map(test_simple_batch);
  if (!test_robust_scaled_or.ok()) {
    std::cerr << "RobustScaler.map(test) failed: " << test_robust_scaled_or.status().message << "\n";
    return 1;
  }
  Series test_robust_scaled = test_robust_scaled_or.move_value();

  prep::MaxAbsScaler maxabs_scaler(prep::MaxAbsScalerOptions{.floor = 1e-12});
  StatusOr<Series> train_maxabs_scaled_or = maxabs_scaler.learn_map(train_simple_batch);
  if (!train_maxabs_scaled_or.ok()) {
    std::cerr << "MaxAbsScaler.learn_map(train) failed: " << train_maxabs_scaled_or.status().message << "\n";
    return 1;
  }
  Series train_maxabs_scaled = train_maxabs_scaled_or.move_value();

  StatusOr<Series> test_maxabs_scaled_or = maxabs_scaler.map(test_simple_batch);
  if (!test_maxabs_scaled_or.ok()) {
    std::cerr << "MaxAbsScaler.map(test) failed: " << test_maxabs_scaled_or.status().message << "\n";
    return 1;
  }
  Series test_maxabs_scaled = test_maxabs_scaled_or.move_value();

  // This section saves compact prep-state diagnostics.
  StatusOr<std::vector<Series>> simple_state_or = simple_imputer.dump_state();
  if (!simple_state_or.ok()) {
    std::cerr << "SimpleImputer.dump_state failed: " << simple_state_or.status().message << "\n";
    return 1;
  }
  std::vector<Series> simple_state = simple_state_or.move_value();

  StatusOr<std::vector<Series>> knn_state_or = knn_imputer.dump_state();
  if (!knn_state_or.ok()) {
    std::cerr << "NeighborImputer.dump_state failed: " << knn_state_or.status().message << "\n";
    return 1;
  }
  std::vector<Series> knn_state = knn_state_or.move_value();

  StatusOr<std::vector<Series>> iter_state_or = iterative_imputer.dump_state();
  if (!iter_state_or.ok()) {
    std::cerr << "IterativeImputer.dump_state failed: " << iter_state_or.status().message << "\n";
    return 1;
  }
  std::vector<Series> iter_state = iter_state_or.move_value();

  StatusOr<std::vector<Series>> standard_state_or = standard_scaler.dump_state();
  if (!standard_state_or.ok()) {
    std::cerr << "StandardScaler.dump_state failed: " << standard_state_or.status().message << "\n";
    return 1;
  }
  std::vector<Series> standard_state = standard_state_or.move_value();

  StatusOr<std::vector<Series>> robust_state_or = robust_scaler.dump_state();
  if (!robust_state_or.ok()) {
    std::cerr << "RobustScaler.dump_state failed: " << robust_state_or.status().message << "\n";
    return 1;
  }
  std::vector<Series> robust_state = robust_state_or.move_value();

  StatusOr<std::vector<Series>> maxabs_state_or = maxabs_scaler.dump_state();
  if (!maxabs_state_or.ok()) {
    std::cerr << "MaxAbsScaler.dump_state failed: " << maxabs_state_or.status().message << "\n";
    return 1;
  }
  std::vector<Series> maxabs_state = maxabs_state_or.move_value();

  DataFrame prep_state_frame({
      {"simple_state_entries", Slate(pluss::make_series(std::vector<double>{static_cast<double>(simple_state.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"knn_state_entries", Slate(pluss::make_series(std::vector<double>{static_cast<double>(knn_state.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"iter_state_entries", Slate(pluss::make_series(std::vector<double>{static_cast<double>(iter_state.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"standard_state_entries", Slate(pluss::make_series(std::vector<double>{static_cast<double>(standard_state.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"robust_state_entries", Slate(pluss::make_series(std::vector<double>{static_cast<double>(robust_state.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"maxabs_state_entries", Slate(pluss::make_series(std::vector<double>{static_cast<double>(maxabs_state.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
  });
  prep_state_frame.save_csv((out_dir / "prep_state_summary.csv").string());

  //========== modelling setup ========
  // This section constructs train/test labeled batches from preprocessed matrices.
  LabeledBatch train_batch{};
  train_batch.features = FeatureBatch{.x = train_standard_scaled, .layout = ObservationLayout::Rows};
  train_batch.y = train_y.to(train_standard_scaled.platform(), ScalarType::Float64).reshape({train_standard_scaled.sizes()[0]});
  train_batch.sample_weights = pluss::series::undefined();

  LabeledBatch test_batch{};
  test_batch.features = FeatureBatch{.x = test_standard_scaled, .layout = ObservationLayout::Rows};
  test_batch.y = test_y.to(test_standard_scaled.platform(), ScalarType::Float64).reshape({test_standard_scaled.sizes()[0]});
  test_batch.sample_weights = pluss::series::undefined();

  Status train_batch_check = validate_labeled(train_batch);
  if (!train_batch_check.ok()) {
    std::cerr << "validate_labeled(train_batch) failed: " << train_batch_check.message << "\n";
    return 1;
  }
  Status test_batch_check = validate_labeled(test_batch);
  if (!test_batch_check.ok()) {
    std::cerr << "validate_labeled(test_batch) failed: " << test_batch_check.message << "\n";
    return 1;
  }

  TrainOptions train_options{};
  train_options.max_iterations = 3000;
  train_options.tolerance = 1e-8;
  train_options.random.seed = 7;
  train_options.random.deterministic = true;

  AssessOptions assess_options{};
  assess_options.metrics = {MetricKind::Rmse, MetricKind::R2, MetricKind::Mae};

  SplitOptions cv_split{};
  cv_split.plan = CVPlanKind::CrossVal;
  cv_split.folds = 5;
  cv_split.shuffle = true;
  cv_split.random.seed = 73;
  cv_split.random.deterministic = true;
  // Force sequential fold evaluation: the shared ParallelRuntime singleton used by
  // pluss::msharp::parallel_for is not safe for concurrent top-level callers, and both
  // cross_assess (fold-level) and sweep (trial-level) can otherwise dispatch concurrently.
  cv_split.max_parallel = 1;

  //========== modelling: broad candidate sweep ========
  // This section runs broad ModelSpec sweep with CV over the preprocessed train batch.
  std::vector<ModelSpec> broad_specs{};

  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::None,
          .regularization_strength = 0.0,
          .fit_intercept = true,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-8,
          .fit_intercept = true,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
  });

  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Tree,
      .options = TreeFamilyOptions{
          .criterion = TreeCriterion::Mse,
          .max_depth = 12,
          .min_samples_leaf = 12,
          .min_samples_split = 30,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Tree,
      .options = TreeFamilyOptions{
          .criterion = TreeCriterion::Mse,
          .max_depth = 18,
          .min_samples_leaf = 8,
          .min_samples_split = 24,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Tree,
      .options = TreeFamilyOptions{
          .criterion = TreeCriterion::Mse,
          .max_depth = 24,
          .min_samples_leaf = 5,
          .min_samples_split = 16,
      },
  });

  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Ensemble,
      .options = EnsembleFamilyOptions{
          .method = EnsembleMethod::RandomForest,
          .learners = 120,
          .subsample = 0.9,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Ensemble,
      .options = EnsembleFamilyOptions{
          .method = EnsembleMethod::RandomForest,
          .learners = 200,
          .subsample = 0.85,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Ensemble,
      .options = EnsembleFamilyOptions{
          .method = EnsembleMethod::Bagging,
          .learners = 180,
          .subsample = 0.95,
      },
  });

  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Neighbor,
      .options = NeighborFamilyOptions{
          .k = 7,
          .distance_weighted = false,
          .standardize = true,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Neighbor,
      .options = NeighborFamilyOptions{
          .k = 9,
          .distance_weighted = true,
          .standardize = true,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Neighbor,
      .options = NeighborFamilyOptions{
          .k = 15,
          .distance_weighted = true,
          .standardize = true,
      },
  });

  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Kernel,
      .options = KernelFamilyOptions{
          .kernel = KernelKind::Rbf,
          .c = 25.0,
          .gamma = 0.5,
          .epsilon = 0.0,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Kernel,
      .options = KernelFamilyOptions{
          .kernel = KernelKind::Rbf,
          .c = 50.0,
          .gamma = 1.0,
          .epsilon = 0.0,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Kernel,
      .options = KernelFamilyOptions{
          .kernel = KernelKind::Polynomial,
          .c = 10.0,
          .gamma = 0.2,
          .epsilon = 0.0,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Kernel,
      .options = KernelFamilyOptions{
          .kernel = KernelKind::Linear,
          .c = 3.0,
          .gamma = 0.0,
          .epsilon = 0.0,
      },
  });

  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Bayes,
      .options = BayesFamilyOptions{
          .use_gaussian = true,
          .smoothing = 1e-9,
      },
  });
  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Bayes,
      .options = BayesFamilyOptions{
          .use_gaussian = true,
          .smoothing = 1e-6,
      },
  });

  StatusOr<TuneReport> broad_sweep_or = sweep(
      broad_specs,
      train_batch,
      cv_split,
      train_options,
      assess_options,
      MetricKind::Rmse,
      TuneObjectivePolicy::Minimize,
      /*max_parallel=*/1);
  if (!broad_sweep_or.ok()) {
    std::cerr << "sweep(broad) failed: " << broad_sweep_or.status().message << "\n";
    return 1;
  }
  TuneReport broad_sweep = broad_sweep_or.move_value();

  std::vector<std::size_t> broad_success_indices{};
  for (std::size_t i = 0; i < broad_sweep.trials.size(); ++i) {
    if (!broad_sweep.trials[i].status.ok()) continue;
    if (!std::isfinite(broad_sweep.trials[i].objective)) continue;
    broad_success_indices.push_back(i);
  }
  if (broad_success_indices.empty()) {
    std::cerr << "broad sweep produced no successful trials.\n";
    return 1;
  }

  std::sort(
      broad_success_indices.begin(),
      broad_success_indices.end(),
      [&](std::size_t a, std::size_t b) {
        return broad_sweep.trials[a].objective < broad_sweep.trials[b].objective;
      });

  std::cout << "\n========== broad cv ranking (rmse, lower is better) ========\n";
  for (std::size_t rank = 0; rank < broad_success_indices.size(); ++rank) {
    const auto idx = broad_success_indices[rank];
    const auto& trial = broad_sweep.trials[idx];
    std::cout << "rank=" << (rank + 1)
              << " candidate_index=" << idx
              << " family=" << static_cast<int>(trial.spec.family)
              << " objective_rmse=" << trial.objective
              << "\n";
  }

  const std::size_t top1_idx = broad_success_indices[0];
  const std::size_t top2_idx = broad_success_indices.size() > 1 ? broad_success_indices[1] : broad_success_indices[0];
  const std::size_t top3_idx = broad_success_indices.size() > 2 ? broad_success_indices[2] : broad_success_indices[0];

  const ModelSpec top1_spec = broad_sweep.trials[top1_idx].spec;
  const ModelSpec top2_spec = broad_sweep.trials[top2_idx].spec;
  const ModelSpec top3_spec = broad_sweep.trials[top3_idx].spec;

  std::cout << "\n========== top three candidates ========\n";
  std::cout << "top1_index=" << top1_idx << " family=" << static_cast<int>(top1_spec.family) << "\n";
  std::cout << "top2_index=" << top2_idx << " family=" << static_cast<int>(top2_spec.family) << "\n";
  std::cout << "top3_index=" << top3_idx << " family=" << static_cast<int>(top3_spec.family) << "\n";

  //========== tuning: focused search around top-three families ========
  // This section builds a denser candidate list around the winning families.
  std::vector<ModelSpec> tuned_specs{};
  std::vector<ModelFamily> top_families = {top1_spec.family, top2_spec.family, top3_spec.family};

  bool include_linear = false;
  bool include_tree = false;
  bool include_ensemble = false;
  bool include_neighbor = false;
  bool include_kernel = false;
  bool include_bayes = false;

  for (const auto fam : top_families) {
    if (fam == ModelFamily::Linear) include_linear = true;
    if (fam == ModelFamily::Tree) include_tree = true;
    if (fam == ModelFamily::Ensemble) include_ensemble = true;
    if (fam == ModelFamily::Neighbor) include_neighbor = true;
    if (fam == ModelFamily::Kernel) include_kernel = true;
    if (fam == ModelFamily::Bayes) include_bayes = true;
  }

  if (include_linear) {
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-9,
            .fit_intercept = true,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-7,
            .fit_intercept = true,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::None,
            .regularization_strength = 0.0,
            .fit_intercept = true,
        },
    });
  }

  if (include_tree) {
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Mse,
            .max_depth = 16,
            .min_samples_leaf = 6,
            .min_samples_split = 20,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Mse,
            .max_depth = 22,
            .min_samples_leaf = 4,
            .min_samples_split = 14,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Mse,
            .max_depth = 28,
            .min_samples_leaf = 3,
            .min_samples_split = 10,
        },
    });
  }

  if (include_ensemble) {
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::RandomForest,
            .learners = 180,
            .subsample = 0.9,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::RandomForest,
            .learners = 260,
            .subsample = 0.85,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::Bagging,
            .learners = 280,
            .subsample = 0.92,
        },
    });
  }

  if (include_neighbor) {
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Neighbor,
        .options = NeighborFamilyOptions{
            .k = 8,
            .distance_weighted = true,
            .standardize = true,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Neighbor,
        .options = NeighborFamilyOptions{
            .k = 12,
            .distance_weighted = true,
            .standardize = true,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Neighbor,
        .options = NeighborFamilyOptions{
            .k = 16,
            .distance_weighted = true,
            .standardize = true,
        },
    });
  }

  if (include_kernel) {
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .c = 40.0,
            .gamma = 0.6,
            .epsilon = 0.0,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .c = 80.0,
            .gamma = 1.2,
            .epsilon = 0.0,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Polynomial,
            .c = 20.0,
            .gamma = 0.3,
            .epsilon = 0.0,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Linear,
            .c = 5.0,
            .gamma = 0.0,
            .epsilon = 0.0,
        },
    });
  }

  if (include_bayes) {
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = true,
            .smoothing = 1e-10,
        },
    });
    tuned_specs.push_back(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = true,
            .smoothing = 1e-8,
        },
    });
  }

  if (tuned_specs.empty()) {
    tuned_specs = broad_specs;
  }

  StatusOr<TuneReport> tuned_sweep_or = sweep(
      tuned_specs,
      train_batch,
      cv_split,
      train_options,
      assess_options,
      MetricKind::Rmse,
      TuneObjectivePolicy::Minimize,
      /*max_parallel=*/1);
  if (!tuned_sweep_or.ok()) {
    std::cerr << "sweep(tuned) failed: " << tuned_sweep_or.status().message << "\n";
    return 1;
  }
  TuneReport tuned_sweep = tuned_sweep_or.move_value();

  std::vector<std::size_t> tuned_success_indices{};
  for (std::size_t i = 0; i < tuned_sweep.trials.size(); ++i) {
    if (!tuned_sweep.trials[i].status.ok()) continue;
    if (!std::isfinite(tuned_sweep.trials[i].objective)) continue;
    tuned_success_indices.push_back(i);
  }
  if (tuned_success_indices.empty()) {
    std::cerr << "tuned sweep produced no successful trials.\n";
    return 1;
  }

  std::sort(
      tuned_success_indices.begin(),
      tuned_success_indices.end(),
      [&](std::size_t a, std::size_t b) {
        return tuned_sweep.trials[a].objective < tuned_sweep.trials[b].objective;
      });

  std::cout << "\n========== tuned cv ranking (rmse, lower is better) ========\n";
  for (std::size_t rank = 0; rank < tuned_success_indices.size(); ++rank) {
    const auto idx = tuned_success_indices[rank];
    const auto& trial = tuned_sweep.trials[idx];
    std::cout << "rank=" << (rank + 1)
              << " candidate_index=" << idx
              << " family=" << static_cast<int>(trial.spec.family)
              << " objective_rmse=" << trial.objective
              << "\n";
  }

  const ModelSpec best_spec = tuned_sweep.trials[tuned_success_indices[0]].spec;

  //========== final model ========
  // This section trains the best model, evaluates holdout, and persists artifacts.
  StatusOr<std::unique_ptr<RegressorBase>> final_reg_or = make_regressor(best_spec);
  if (!final_reg_or.ok()) {
    std::cerr << "make_regressor(best_spec) failed: " << final_reg_or.status().message << "\n";
    return 1;
  }
  std::unique_ptr<RegressorBase> final_reg = final_reg_or.move_value();

  Status final_learn_status = final_reg->learn(train_batch, train_options);
  if (!final_learn_status.ok()) {
    std::cerr << "final_reg.learn failed: " << final_learn_status.message << "\n";
    return 1;
  }

  StatusOr<Series> final_metrics_or = final_reg->assess(test_batch, assess_options);
  if (!final_metrics_or.ok()) {
    std::cerr << "final_reg.assess failed: " << final_metrics_or.status().message << "\n";
    return 1;
  }
  Series final_metrics = final_metrics_or.move_value().to(pluss::pCPU, ScalarType::Float64).reshape({3});
  const std::vector<double> final_metrics_vec = final_metrics.host<double>();

  StatusOr<Series> final_pred_or = final_reg->infer(test_batch.features);
  if (!final_pred_or.ok()) {
    std::cerr << "final_reg.infer failed: " << final_pred_or.status().message << "\n";
    return 1;
  }
  Series final_pred = final_pred_or.move_value();

  std::cout << "\n========== final holdout metrics ========\n";
  std::cout << "rmse=" << (final_metrics_vec.size() > 0 ? final_metrics_vec[0] : std::numeric_limits<double>::quiet_NaN()) << "\n";
  std::cout << "r2=" << (final_metrics_vec.size() > 1 ? final_metrics_vec[1] : std::numeric_limits<double>::quiet_NaN()) << "\n";
  std::cout << "mae=" << (final_metrics_vec.size() > 2 ? final_metrics_vec[2] : std::numeric_limits<double>::quiet_NaN()) << "\n";

  //========== flow persistence with best spec ========
  // This section uses core Flow + io::save/io::load_and_restore for exact pipeline persistence.
  StatusOr<Flow> final_flow_or = make_flow(best_spec);
  if (!final_flow_or.ok()) {
    std::cerr << "make_flow(best_spec) failed: " << final_flow_or.status().message << "\n";
    return 1;
  }
  Flow final_flow = final_flow_or.move_value();

  Status flow_learn_status = final_flow.learn(train_batch, train_options);
  if (!flow_learn_status.ok()) {
    std::cerr << "final_flow.learn failed: " << flow_learn_status.message << "\n";
    return 1;
  }

  const fs::path final_model_path = model_dir / "nodej_hour_end_to_end_final.bin";
  Status flow_save_status = io::save(final_model_path.string(), final_flow, train_batch, train_options);
  if (!flow_save_status.ok()) {
    std::cerr << "io::save failed: " << flow_save_status.message << "\n";
    return 1;
  }

  StatusOr<Flow> restored_flow_or = io::load_and_restore(final_model_path.string());
  if (!restored_flow_or.ok()) {
    std::cerr << "io::load_and_restore failed: " << restored_flow_or.status().message << "\n";
    return 1;
  }
  Flow restored_flow = restored_flow_or.move_value();

  StatusOr<Series> restored_pred_or = restored_flow.infer(test_batch.features);
  if (!restored_pred_or.ok()) {
    std::cerr << "restored_flow.infer failed: " << restored_pred_or.status().message << "\n";
    return 1;
  }
  Series restored_pred = restored_pred_or.move_value();

  const Series pred_cpu = final_pred.to(pluss::pCPU, ScalarType::Float64).reshape({final_pred.numel()});
  const Series restored_cpu = restored_pred.to(pluss::pCPU, ScalarType::Float64).reshape({restored_pred.numel()});
  const std::vector<double> pred_vec = pred_cpu.host<double>();
  const std::vector<double> restored_vec = restored_cpu.host<double>();

  double restored_max_abs_diff = 0.0;
  for (std::size_t i = 0; i < pred_vec.size() && i < restored_vec.size(); ++i) {
    restored_max_abs_diff = std::max(restored_max_abs_diff, std::abs(pred_vec[i] - restored_vec[i]));
  }

  std::cout << "\n========== persistence check ========\n";
  std::cout << "model_path=" << final_model_path << "\n";
  std::cout << "restored_max_abs_diff=" << restored_max_abs_diff << "\n";

  //========== final diagnostics artifacts ========
  // This section exports prediction diagnostics as CSV + plots.
  const Series y_true_cpu = test_batch.y.to(pluss::pCPU, ScalarType::Float64).reshape({test_batch.y.numel()});
  const Series y_pred_cpu = pred_cpu;
  const Series residual_cpu = y_true_cpu.sub(y_pred_cpu);
  const Series abs_error_cpu = residual_cpu.abs();
  const Series sq_error_cpu = residual_cpu.square();

  DataFrame pred_frame({
      {"y_true", Slate(y_true_cpu)},
      {"y_pred", Slate(y_pred_cpu)},
      {"residual", Slate(residual_cpu)},
      {"abs_error", Slate(abs_error_cpu)},
      {"sq_error", Slate(sq_error_cpu)},
  });

  pred_frame.save_csv((out_dir / "final_test_predictions.csv").string());
  pred_frame.describe(false).save_csv((out_dir / "final_prediction_describe.csv").string());
  pred_frame.corr(false).save_csv((out_dir / "final_prediction_corr.csv").string());

  pred_frame.plot()
      .scatter("y_true", "y_pred")
      .headline("Observed vs predicted bike demand (final model)")
      .save_pdf((out_dir / "final_pred_vs_true.pdf").string());
  pred_frame.plot()
      .hist("residual", 80)
      .headline("Residual distribution (final model)")
      .save_pdf((out_dir / "final_residual_hist.pdf").string());
  pred_frame.plot()
      .hist("abs_error", 80)
      .headline("Absolute error distribution (final model)")
      .save_pdf((out_dir / "final_abs_error_hist.pdf").string());

  //========== run summary ========
  // This section writes compact run metadata.
  DataFrame run_summary({
      {"n_rows_loaded", Slate(pluss::make_series(std::vector<double>{static_cast<double>(frame.ncases())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"n_features_used", Slate(pluss::make_series(std::vector<double>{static_cast<double>(model_feature_names.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"holdout_train_rows", Slate(pluss::make_series(std::vector<double>{static_cast<double>(train_frame_full.ncases())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"holdout_test_rows", Slate(pluss::make_series(std::vector<double>{static_cast<double>(test_frame_full.ncases())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"broad_candidates", Slate(pluss::make_series(std::vector<double>{static_cast<double>(broad_specs.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"tuned_candidates", Slate(pluss::make_series(std::vector<double>{static_cast<double>(tuned_specs.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"final_rmse", Slate(pluss::make_series(std::vector<double>{final_metrics_vec.size() > 0 ? final_metrics_vec[0] : std::numeric_limits<double>::quiet_NaN()}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"final_r2", Slate(pluss::make_series(std::vector<double>{final_metrics_vec.size() > 1 ? final_metrics_vec[1] : std::numeric_limits<double>::quiet_NaN()}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"final_mae", Slate(pluss::make_series(std::vector<double>{final_metrics_vec.size() > 2 ? final_metrics_vec[2] : std::numeric_limits<double>::quiet_NaN()}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"restored_max_abs_diff", Slate(pluss::make_series(std::vector<double>{restored_max_abs_diff}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
  });
  run_summary.save_csv((out_dir / "run_summary.csv").string());

  std::cout << "\n========== completed ========\n";
  std::cout << "saved_outputs=" << out_dir << "\n";
  std::cout << "saved_model=" << final_model_path << "\n";
  return 0;
}
