#include <pluss/pluss.h>
#include <pluss/tils.h>
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

int main() {
  namespace fs = std::filesystem;
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::pCPU;
  using pluss::table::DataFrame;
  using pluss::table::Thread;
  pluss::OutputEngine out;
  using Clock = std::chrono::steady_clock;
  auto secs = [](Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
  };

  //========== Cell 1: Load Data ========
  // Mirror chapter-2 housing dataset setup.
  const fs::path data_path = fs::path(PLUSSMIC_SOURCE_DIR) / "data" / "housing.csv";
  const fs::path output_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "outputs" / "lymph_housing_ch2";
  const fs::path model_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "NodeJ" / "trained_models";
  fs::create_directories(output_dir);
  fs::create_directories(model_dir);

  DataFrame housing = DataFrame::load_csv(data_path.string());
  out.writef("rows={} features={}", housing.ncases(), housing.nfeatures()).endln();
  out.writeln("feature names", housing.features());
  housing.head(5);
  //========== End Cell 1 ========

  //========== Cell 2: Stratified Holdout Split ========
  // Equivalent in spirit to sklearn's income category stratified split.
  housing.set("income_cat") = housing.features("median_income").bucketize(
      {0.0, 1.5, 3.0, 4.5, 6.0, std::numeric_limits<double>::infinity()},
      {1, 2, 3, 4, 5});

  auto [strat_train_set, strat_test_set] = lymph::hold_out_split(
      housing,
      lymph::HoldoutSplitOptions{
          .test_size = 0.2,
          .stratify_feature = "income_cat",
          .random_state = 42,
          .deterministic = true,
      });

  strat_train_set = strat_train_set.drop({"income_cat"});
  strat_test_set = strat_test_set.drop({"income_cat"});
  out.writef("train rows={} test rows={}", strat_train_set.ncases(), strat_test_set.ncases()).endln();
  //========== End Cell 2 ========

  //========== Cell 3: EDA + Correlation Snapshot ========
  // Quick, notebook-style checks and one correlation ranking view.
  auto corr_ranked =
      strat_train_set.feataxis().series().corr().arrange({"median_house_value"}, false).features({"median_house_value"});
  out.writeln("top correlations with median_house_value");
  corr_ranked.head(10);
  //========== End Cell 3 ========

  //========== Cell 4: Feature Engineering ========
  // Chapter-2 style engineered ratios.
  strat_train_set.set({"rooms_per_house", "bedrooms_ratio", "people_per_house"}) = {
      strat_train_set.features("total_rooms") / strat_train_set.features("households"),
      strat_train_set.features("total_bedrooms") / strat_train_set.features("total_rooms"),
      strat_train_set.features("population") / strat_train_set.features("households"),
  };
  strat_test_set.set({"rooms_per_house", "bedrooms_ratio", "people_per_house"}) = {
      strat_test_set.features("total_rooms") / strat_test_set.features("households"),
      strat_test_set.features("total_bedrooms") / strat_test_set.features("total_rooms"),
      strat_test_set.features("population") / strat_test_set.features("households"),
  };
  //========== End Cell 4 ========

  //========== Cell 5: Split X/Y + Numeric/Categorical Views ========
  // This is the explicit DataFrame split analogous to pandas X/y selection.
  const DataFrame train_x_raw = strat_train_set.drop({"median_house_value"});
  const Thread train_y = strat_train_set.features("median_house_value");
  const DataFrame test_x_raw = strat_test_set.drop({"median_house_value"});
  const Thread test_y = strat_test_set.features("median_house_value");

  const DataFrame train_num = train_x_raw.feataxis().series();
  const DataFrame test_num = test_x_raw.feataxis().series();
  const DataFrame train_cat = train_x_raw.feataxis().note();
  const DataFrame test_cat = test_x_raw.feataxis().note();
  const auto categorical_names = train_cat.features();
  out.writef("numeric feature count={} categorical feature count={}", train_num.nfeatures(), train_cat.nfeatures()).endln();
  if (train_num.nfeatures() == 0) {
    throw std::runtime_error("chapter2 demo: no numeric features resolved.");
  }
  if (train_cat.nfeatures() == 0) {
    throw std::runtime_error("chapter2 demo: no categorical features resolved.");
  }
  if (categorical_names.empty()) {
    throw std::runtime_error("chapter2 demo: categorical feature list is empty.");
  }
  //========== End Cell 5 ========

  //========== Cell 6: Impute Numeric ========
  // sklearn SimpleImputer(strategy='median') equivalent.
  auto num_imputer = lymph::prep::SimpleImputer(lymph::prep::SimpleImputerOptions{
      .strategy = lymph::prep::SimpleImputerStrategy::Median,
  });
  const DataFrame train_num_imputed = num_imputer.learn_map(train_num);
  const DataFrame test_num_imputed = num_imputer.map(test_num);
  out.writeln("imputer summary", num_imputer.summary());
  //========== End Cell 6 ========

  //========== Cell 7: Scale Numeric ========
  // sklearn StandardScaler equivalent.
  auto num_scaler = lymph::prep::StandardScaler(lymph::prep::StandardScalerOptions{
      .with_centering = true,
      .with_scaling = true,
  });
  const DataFrame train_num_scaled = num_scaler.learn_map(train_num_imputed);
  const DataFrame test_num_scaled = num_scaler.map(test_num_imputed);
  out.writeln("scaler summary", num_scaler.summary());
  //========== End Cell 7 ========

  //========== Cell 8: One-Hot Encode Categorical ========
  // sklearn OneHotEncoder equivalent with unknown bucket retained.
  auto cat_encoder = lymph::prep::OneHotEncoder(lymph::prep::OneHotEncoderOptions{
      .max_categories = 32,
      .include_unknown_bucket = true,
  });
  cat_encoder.features({"ocean_proximity"});
  const DataFrame train_cat_1hot = cat_encoder.learn_map(train_cat);
  const DataFrame test_cat_1hot = cat_encoder.map(test_cat);
  out.writeln("one-hot categories_", cat_encoder.categories_);
  out.writeln("one-hot mapped_names_", cat_encoder.mapped_names_);
  out.writef("one-hot train features={} one-hot test features={}", train_cat_1hot.nfeatures(), test_cat_1hot.nfeatures()).endln();
  //========== End Cell 8 ========

  //========== Cell 9: Prepared Matrix For CV/Tuning ========
  const DataFrame train_prepared = train_num_scaled.combine(train_cat_1hot);
  const DataFrame test_prepared = test_num_scaled.combine(test_cat_1hot);
  out.writef("prepared train features={} prepared test features={}", train_prepared.nfeatures(), test_prepared.nfeatures()).endln();
  //========== End Cell 9 ========

  //========== Cell 10: HARD GAP Notes ========
  // HARD GAP: no direct TransformedTargetRegressor-style wrapper in Lymph.
  // HARD GAP: no HalvingRandomSearchCV equivalent exposed as a single API today.
  // HARD GAP: no built-in RMSE confidence-interval utility; done manually below.
  //========== End Cell 10 ========

  //========== Cell 11: Train Baselines ========
  // Linear Regression baseline.
  auto linear_wf = lymph::models::linear_regression(pluss::ai::nodej::LinearFamilyOptions{
      .objective = pluss::ai::nodej::LinearObjective::LeastSquares,
      .regularization = pluss::ai::nodej::RegularizationKind::None,
      .regularization_strength = 0.0,
      .fit_intercept = true,
  });

  // Decision Tree baseline.
  auto tree_wf = lymph::models::decision_tree_regressor(pluss::ai::nodej::TreeFamilyOptions{
      .criterion = pluss::ai::nodej::TreeCriterion::Mse,
      .max_depth = 18,
      .min_samples_leaf = 4,
      .min_samples_split = 8,
  });

  // Random Forest baseline.
  auto forest_wf = lymph::models::random_forest_regressor(pluss::ai::nodej::EnsembleFamilyOptions{
      .method = pluss::ai::nodej::EnsembleMethod::RandomForest,
      .learners = 120,
      .subsample = 1.0,
  });

  pluss::ai::nodej::TrainOptions train_options{};
  train_options.max_iterations = 2500;
  train_options.tolerance = 1e-8;
  train_options.random.seed = 42;
  train_options.random.deterministic = true;

  pluss::ai::nodej::AssessOptions assess_options{};
  assess_options.metrics = {
      pluss::ai::nodej::MetricKind::Rmse,
      pluss::ai::nodej::MetricKind::R2,
      pluss::ai::nodej::MetricKind::Mae,
  };

  const auto t_lin0 = Clock::now();
  linear_wf.learn_(train_prepared, train_y, train_options);
  const auto t_lin1 = Clock::now();
  out.writef("linear learn seconds={}", secs(t_lin0, t_lin1)).endln();

  const auto t_tree0 = Clock::now();
  tree_wf.learn_(train_prepared, train_y, train_options);
  const auto t_tree1 = Clock::now();
  out.writef("tree learn seconds={}", secs(t_tree0, t_tree1)).endln();

  const auto t_forest0 = Clock::now();
  forest_wf.learn_(train_prepared, train_y, train_options);
  const auto t_forest1 = Clock::now();
  out.writef("forest learn seconds={}", secs(t_forest0, t_forest1)).endln();

  const auto linear_test_metrics = linear_wf.assess(test_prepared, test_y, assess_options);
  const auto tree_test_metrics = tree_wf.assess(test_prepared, test_y, assess_options);
  const auto forest_test_metrics = forest_wf.assess(test_prepared, test_y, assess_options);

  out.writeln("linear test", linear_test_metrics.rmse, linear_test_metrics.r2, linear_test_metrics.mae);
  out.writeln("tree test", tree_test_metrics.rmse, tree_test_metrics.r2, tree_test_metrics.mae);
  out.writeln("forest test", forest_test_metrics.rmse, forest_test_metrics.r2, forest_test_metrics.mae);
  //========== End Cell 11 ========

  //========== Cell 12: Cross-Validation Comparison ========
  // Equivalent in spirit to sklearn cross_val_score model ranking.
  lymph::CrossValidationOptions cv_options{};
  cv_options.plan = pluss::ai::nodej::CVPlanKind::CrossVal;
  cv_options.folds = 3;
  cv_options.shuffle = true;
  cv_options.random_state = 42;
  cv_options.deterministic = true;

  const auto t_cv0 = Clock::now();
  const auto linear_cv = lymph::cross_validate(linear_wf.model_spec().value(), train_prepared, train_y, cv_options, train_options, assess_options);
  const auto tree_cv = lymph::cross_validate(tree_wf.model_spec().value(), train_prepared, train_y, cv_options, train_options, assess_options);
  const auto forest_cv = lymph::cross_validate(forest_wf.model_spec().value(), train_prepared, train_y, cv_options, train_options, assess_options);
  const auto t_cv1 = Clock::now();
  out.writef("cross_validate block seconds={}", secs(t_cv0, t_cv1)).endln();

  out.writeln("linear cv", linear_cv.mean.rmse, linear_cv.mean.r2);
  out.writeln("tree cv", tree_cv.mean.rmse, tree_cv.mean.r2);
  out.writeln("forest cv", forest_cv.mean.rmse, forest_cv.mean.r2);
  //========== End Cell 12 ========

  //========== Cell 13: Tune Candidate Families ========
  // Equivalent in spirit to Grid/Random search over model hyperparameters.
  std::vector<lymph::selection::Candidate> candidates{};
  candidates.emplace_back("linear_l2_1e6", lymph::models::linear_regression(pluss::ai::nodej::LinearFamilyOptions{
      .objective = pluss::ai::nodej::LinearObjective::LeastSquares,
      .regularization = pluss::ai::nodej::RegularizationKind::L2,
      .regularization_strength = 1e-6,
      .fit_intercept = true,
  }));
  candidates.emplace_back("linear_l2_1e4", lymph::models::linear_regression(pluss::ai::nodej::LinearFamilyOptions{
      .objective = pluss::ai::nodej::LinearObjective::LeastSquares,
      .regularization = pluss::ai::nodej::RegularizationKind::L2,
      .regularization_strength = 1e-4,
      .fit_intercept = true,
  }));
  candidates.emplace_back("tree_d12_leaf2", lymph::models::decision_tree_regressor(pluss::ai::nodej::TreeFamilyOptions{
      .criterion = pluss::ai::nodej::TreeCriterion::Mse,
      .max_depth = 12,
      .min_samples_leaf = 2,
      .min_samples_split = 8,
  }));
  candidates.emplace_back("tree_d24_leaf8", lymph::models::decision_tree_regressor(pluss::ai::nodej::TreeFamilyOptions{
      .criterion = pluss::ai::nodej::TreeCriterion::Mse,
      .max_depth = 24,
      .min_samples_leaf = 8,
      .min_samples_split = 20,
  }));
  candidates.emplace_back("forest_200_sub09", lymph::models::random_forest_regressor(pluss::ai::nodej::EnsembleFamilyOptions{
      .method = pluss::ai::nodej::EnsembleMethod::RandomForest,
      .learners = 80,
      .subsample = 0.9,
  }));
  candidates.emplace_back("forest_500_sub10", lymph::models::random_forest_regressor(pluss::ai::nodej::EnsembleFamilyOptions{
      .method = pluss::ai::nodej::EnsembleMethod::RandomForest,
      .learners = 140,
      .subsample = 1.0,
  }));

  const auto t_tune0 = Clock::now();
  auto tuned = lymph::selection::compare(
      std::move(candidates),
      train_prepared,
      train_y,
      cv_options,
      train_options,
      assess_options,
      pluss::ai::nodej::MetricKind::Rmse,
      pluss::ai::nodej::TuneObjectivePolicy::Minimize);
  const auto t_tune1 = Clock::now();
  out.writef("tune block seconds={}", secs(t_tune0, t_tune1)).endln();

  out.writeln("best candidate", tuned.report.best_name);
  out.writeln("trial count", static_cast<std::int64_t>(tuned.report.trials.size()));
  //========== End Cell 13 ========

  //========== Cell 14: Final Model Eval + RMSE CI (Manual) ========
  // Manual CI computation to mirror chapter narrative.
  auto final_workflow = std::move(tuned.best());
  const Series final_pred = final_workflow.infer(test_prepared);
  const auto final_metrics = final_workflow.assess(test_prepared, test_y, assess_options);
  out.writeln("final test", final_metrics.rmse, final_metrics.r2, final_metrics.mae);

  const Series y_test_series = test_y.as_series().to(pCPU, ScalarType::Float64).reshape({test_y.size()});
  const auto squared_errors =
      final_pred.to(pCPU, ScalarType::Float64).reshape({final_pred.numel()}).sub(y_test_series).square().to_vec<double>();
  const std::size_t n = squared_errors.size();
  double mean_sq = 0.0;
  for (double v : squared_errors) mean_sq += v;
  mean_sq /= static_cast<double>(std::max<std::size_t>(n, 1));
  double var_sq = 0.0;
  for (double v : squared_errors) {
    const double d = v - mean_sq;
    var_sq += d * d;
  }
  var_sq /= static_cast<double>(std::max<std::size_t>(n > 1 ? (n - 1) : 1, 1));
  const double se_sq = std::sqrt(var_sq / static_cast<double>(std::max<std::size_t>(n, 1)));
  const double z95 = 1.96;
  const double rmse_mean = std::sqrt(std::max(0.0, mean_sq));
  const double rmse_lo = std::sqrt(std::max(0.0, mean_sq - z95 * se_sq));
  const double rmse_hi = std::sqrt(std::max(0.0, mean_sq + z95 * se_sq));
  out.writeln("approx 95% rmse interval", std::vector<double>{rmse_lo, rmse_hi}, "rmse_point", rmse_mean);
  //========== End Cell 14 ========

  //========== Cell 15: Save, Load, Re-Infer ========
  const fs::path final_model_path = model_dir / "lymph_housing_ch2_best.bin";
  final_workflow.save(final_model_path.string(), train_prepared, train_y, train_options);
  auto restored = lymph::Model::load(final_model_path.string());
  const Series restored_pred = restored.infer(test_prepared);
  const auto parity = lymph::compare_series(final_pred, restored_pred);
  out.writeln("restored parity", parity.max_abs_diff, parity.mae);

  DataFrame prediction_frame({
      {"y_true", test_y},
      {"y_pred", Thread(final_pred)},
      {"y_pred_restored", Thread(restored_pred)},
  });
  prediction_frame.save_csv((output_dir / "housing_ch2_test_predictions.csv").string());
  out.writeln("saved predictions", (output_dir / "housing_ch2_test_predictions.csv").string());
  out.writeln("saved model", final_model_path.string());
  //========== End Cell 15 ========

  return 0;
}
