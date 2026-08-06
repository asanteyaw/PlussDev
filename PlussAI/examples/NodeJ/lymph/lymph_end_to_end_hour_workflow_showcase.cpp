#include <pluss/ml.h>
#include <pluss/mic.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
  using DataFrame = pluss::table::DataFrame;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::Thread;
  using namespace pluss::ai::nodej;
  namespace lymph = pluss::ai::lymph;

  try {
    //========== setup: paths, run controls, and output locations ========
    // This section controls where data comes from, how many rows are used,
    // and where artifacts (plots/csv/models) are written.
    fs::path data_path = fs::path(PLUSSMIC_SOURCE_DIR) / "data" / "hour.csv";
    fs::path out_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "outputs" / "lymph_end_to_end_hour_workflow_showcase";
    fs::path model_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "NodeJ" / "trained_models";
    std::int64_t ncases = 14000;
    if (argc > 1 && argv[1] != nullptr && std::string(argv[1]).size() > 0) data_path = fs::path(argv[1]);
    if (argc > 2 && argv[2] != nullptr && std::string(argv[2]).size() > 0) out_dir = fs::path(argv[2]);
    if (argc > 3 && argv[3] != nullptr && std::string(argv[3]).size() > 0) ncases = std::stoll(argv[3]);
    fs::create_directories(out_dir);
    fs::create_directories(model_dir);

    //========== data loading ========
    // This section loads raw bike-sharing hourly data from PlussMic.
    const DataFrame raw = DataFrame::load_csv(data_path.string(), {{"ncases", ncases}});

    //========== schema normalization ========
    // This section makes feature names explicit and consistent for the ML workflow.
    auto frame = raw.rename({
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


    //========== feature engineering ========
    // This section creates interaction and curvature features directly in DataFrame form
    // to expose raw lymph + DataFrame capability without helper functions.
    const Series s_temp = frame.features("temp").as_series();
    const Series s_atemp = frame.features("atemp").as_series();
    const Series s_humidity = frame.features("humidity").as_series();
    const Series s_windspeed = frame.features("windspeed").as_series();
    const Series s_hour = frame.features("hour").as_series();
    const Series s_month = frame.features("month").as_series();
    const Series s_weather = frame.features("weather_condition").as_series();

    const Series temp_sq = s_temp.square();
    const Series atemp_sq = s_atemp.square();
    const Series humidity_sq = s_humidity.square();
    const Series windspeed_sq = s_windspeed.square();
    const Series temp_humidity_interaction = s_temp.mul(s_humidity);
    const Series temp_windspeed_interaction = s_temp.mul(s_windspeed);
    const Series atemp_humidity_interaction = s_atemp.mul(s_humidity);
    const Series atemp_windspeed_interaction = s_atemp.mul(s_windspeed);
    const Series temp_minus_atemp = s_temp.sub(s_atemp);
    const Series hour_centered = s_hour.sub(12.0);
    const Series hour_centered_sq = hour_centered.square();
    const Series month_centered = s_month.sub(6.5);
    const Series month_centered_sq = month_centered.square();
    const Series weather_sq = s_weather.square();

    frame.set("temp_sq") = Thread(temp_sq);
    frame.set("atemp_sq") = Thread(atemp_sq);
    frame.set("humidity_sq") = Thread(humidity_sq);
    frame.set("windspeed_sq") = Thread(windspeed_sq);
    frame.set("temp_humidity_interaction") = Thread(temp_humidity_interaction);
    frame.set("temp_windspeed_interaction") = Thread(temp_windspeed_interaction);
    frame.set("atemp_humidity_interaction") = Thread(atemp_humidity_interaction);
    frame.set("atemp_windspeed_interaction") = Thread(atemp_windspeed_interaction);
    frame.set("temp_minus_atemp") = Thread(temp_minus_atemp);
    frame.set("hour_centered") = Thread(hour_centered);
    frame.set("hour_centered_sq") = Thread(hour_centered_sq);
    frame.set("month_centered") = Thread(month_centered);
    frame.set("month_centered_sq") = Thread(month_centered_sq);
    frame.set("weather_sq") = Thread(weather_sq);

    //========== missingness diagnostics ========
    // This section checks NA patterns before preprocessing to verify data quality.
    const DataFrame missing_binary = frame.isnull({
        "temp", "atemp", "humidity", "windspeed", "total_count",
        "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
    });
    const auto missing_rate = missing_binary.mean(pluss::table::Axis::Case, false);
    const DataFrame missing_rate_table(
        {{"missing_rate", missing_rate}},
        Thread(pluss::make_note(missing_binary.features())));
    missing_rate_table.save_csv((out_dir / "missing_rate.csv").string());

    //========== exploratory data analysis (EDA) ========
    // This section computes descriptive statistics and correlation matrix,
    // then writes the tables for auditability.
    const DataFrame numeric_block = frame.features({
        "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition",
        "temp", "atemp", "humidity", "windspeed", "casual", "registered", "total_count",
        "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
        "temp_humidity_interaction", "temp_windspeed_interaction", "atemp_humidity_interaction", "atemp_windspeed_interaction",
        "temp_minus_atemp", "hour_centered", "hour_centered_sq", "month_centered", "month_centered_sq", "weather_sq",
    });
    const DataFrame describe_table = numeric_block.describe(false);
    const DataFrame corr_table = numeric_block.corr(false);
    describe_table.save_csv((out_dir / "eda_describe.csv").string());
    corr_table.save_csv((out_dir / "eda_corr.csv").string());

    // This section produces compact but useful EDA visual diagnostics.
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

    const DataFrame hour_mean = frame.group({.dropnull = true})
                                    .by({"hour"})
                                    .agg({
                                        {"mean_total_count", "total_count", DataFrame::GroupOp::Mean},
                                        {"count", "total_count", DataFrame::GroupOp::Count},
                                    });
    const DataFrame weather_mean = frame.group({.dropnull = true})
                                       .by({"weather_condition"})
                                       .agg({
                                           {"mean_total_count", "total_count", DataFrame::GroupOp::Mean},
                                           {"count", "total_count", DataFrame::GroupOp::Count},
                                       });
    const DataFrame month_mean = frame.group({.dropnull = true})
                                     .by({"month"})
                                     .agg({
                                         {"mean_total_count", "total_count", DataFrame::GroupOp::Mean},
                                         {"count", "total_count", DataFrame::GroupOp::Count},
                                     });
    const DataFrame weekday_mean = frame.group({.dropnull = true})
                                       .by({"weekday"})
                                       .agg({
                                           {"mean_total_count", "total_count", DataFrame::GroupOp::Mean},
                                           {"count", "total_count", DataFrame::GroupOp::Count},
                                       });

    hour_mean.save_csv((out_dir / "eda_hour_mean_total_count.csv").string());
    weather_mean.save_csv((out_dir / "eda_weather_mean_total_count.csv").string());
    month_mean.save_csv((out_dir / "eda_month_mean_total_count.csv").string());
    weekday_mean.save_csv((out_dir / "eda_weekday_mean_total_count.csv").string());

    hour_mean.plot()
        .line("mean_total_count", "hour")
        .headline("Average rentals by hour")
        .save_pdf((out_dir / "eda_hour_mean_line.pdf").string());
    month_mean.plot()
        .line("mean_total_count", "month")
        .headline("Average rentals by month")
        .save_pdf((out_dir / "eda_month_mean_line.pdf").string());
    weather_mean.plot()
        .bar("mean_total_count", "weather_condition")
        .headline("Average rentals by weather condition")
        .save_pdf((out_dir / "eda_weather_mean_bar.pdf").string());

    //========== train/test split ========
    // This section creates a deterministic holdout split for final validation.
    const auto holdout = lymph::holdout_split(frame, lymph::HoldoutSplitOptions{
                                                         .test_size = 0.2,
                                                         .random_state = 42,
                                                         .deterministic = true,
                                                         .shuffle = true,
                                                     });

    const std::vector<std::string> model_features = {
        "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition",
        "temp", "atemp", "humidity", "windspeed", "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
        "temp_humidity_interaction", "temp_windspeed_interaction", "atemp_humidity_interaction", "atemp_windspeed_interaction",
        "temp_minus_atemp", "hour_centered", "hour_centered_sq", "month_centered", "month_centered_sq", "weather_sq",
    };

    const DataFrame train_x = holdout.train_set.features({
        "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition",
        "temp", "atemp", "humidity", "windspeed", "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
        "temp_humidity_interaction", "temp_windspeed_interaction", "atemp_humidity_interaction", "atemp_windspeed_interaction",
        "temp_minus_atemp", "hour_centered", "hour_centered_sq", "month_centered", "month_centered_sq", "weather_sq",
    });
    const DataFrame test_x = holdout.test_set.features({
        "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition",
        "temp", "atemp", "humidity", "windspeed", "temp_sq", "atemp_sq", "humidity_sq", "windspeed_sq",
        "temp_humidity_interaction", "temp_windspeed_interaction", "atemp_humidity_interaction", "atemp_windspeed_interaction",
        "temp_minus_atemp", "hour_centered", "hour_centered_sq", "month_centered", "month_centered_sq", "weather_sq",
    });
    const Series train_y = holdout.train_set.features("total_count").as_series();
    const Series test_y = holdout.test_set.features("total_count").as_series();

    //========== standalone lymph preprocessing API ========
    // This section demonstrates direct prep estimators (learn/map/learn_map)
    // without going through Workflow composition.
    auto simple_imp = lymph::prep::simple_imputer(lymph::prep::SimpleImputerOptions{
        .strategy = lymph::prep::SimpleImputerStrategy::Median,
    });
    const DataFrame train_simple_imputed = simple_imp.learn_map(train_x);
    const DataFrame test_simple_imputed = simple_imp.map(test_x);
    const auto simple_imp_summary = simple_imp.summary();

    auto knn_imp = lymph::prep::neighbor_imputer(lymph::prep::NeighborImputerOptions{
        .neighbors = 7,
        .weighting = lymph::prep::NeighborImputerWeighting::Distance,
        .distance_floor = 1e-12,
        .empty_feature_fallback = 0.0,
    });
    const DataFrame train_knn_imputed = knn_imp.learn_map(train_x);
    const DataFrame test_knn_imputed = knn_imp.map(test_x);
    const auto knn_imp_summary = knn_imp.summary();

    auto iter_imp = lymph::prep::iterative_imputer(lymph::prep::IterativeImputerOptions{
        .rounds = 6,
        .ridge = 1e-5,
        .empty_feature_fallback = 0.0,
    });
    const DataFrame train_iter_imputed = iter_imp.learn_map(train_x);
    const DataFrame test_iter_imputed = iter_imp.map(test_x);
    const auto iter_imp_summary = iter_imp.summary();

    auto z_scaler = lymph::prep::standard_scaler(lymph::prep::StandardScalerOptions{
        .with_centering = true,
        .with_scaling = true,
        .variance_floor = 1e-12,
    });
    const DataFrame train_scaled = z_scaler.learn_map(train_simple_imputed);
    const DataFrame test_scaled = z_scaler.map(test_simple_imputed);
    const auto z_summary = z_scaler.summary();

    auto robust_scaler = lymph::prep::robust_scaler(lymph::prep::RobustScalerOptions{
        .with_centering = true,
        .with_scaling = true,
        .lower_quantile = 0.15,
        .upper_quantile = 0.85,
        .range_floor = 1e-9,
    });
    const DataFrame train_robust_scaled = robust_scaler.learn_map(train_simple_imputed);
    const DataFrame test_robust_scaled = robust_scaler.map(test_simple_imputed);
    const auto robust_summary = robust_scaler.summary();

    auto maxabs_scaler = lymph::prep::maxabs_scaler(lymph::prep::MaxAbsScalerOptions{.floor = 1e-12});
    const DataFrame train_maxabs_scaled = maxabs_scaler.learn_map(train_simple_imputed);
    const DataFrame test_maxabs_scaled = maxabs_scaler.map(test_simple_imputed);
    const auto maxabs_summary = maxabs_scaler.summary();

    // This section persists preprocessing outputs for inspection.
    train_simple_imputed.save_csv((out_dir / "prep_train_simple_imputed.csv").string());
    train_knn_imputed.save_csv((out_dir / "prep_train_knn_imputed.csv").string());
    train_iter_imputed.save_csv((out_dir / "prep_train_iter_imputed.csv").string());
    train_scaled.save_csv((out_dir / "prep_train_standard_scaled.csv").string());
    train_robust_scaled.save_csv((out_dir / "prep_train_robust_scaled.csv").string());
    train_maxabs_scaled.save_csv((out_dir / "prep_train_maxabs_scaled.csv").string());

    // This section stores compact preprocessing state snapshots as CSV.
    DataFrame prep_state_frame({
        {"simple_imputer_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(simple_imp_summary.feature_names.size())}, {1}))},
        {"knn_imputer_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(knn_imp_summary.feature_names.size())}, {1}))},
        {"iter_imputer_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(iter_imp_summary.feature_names.size())}, {1}))},
        {"standard_scaler_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(z_summary.feature_names.size())}, {1}))},
        {"robust_scaler_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(robust_summary.feature_names.size())}, {1}))},
        {"maxabs_scaler_state_cols", Thread(pluss::make_series(std::vector<double>{static_cast<double>(maxabs_summary.feature_names.size())}, {1}))},
    });
    prep_state_frame.save_csv((out_dir / "prep_state_summary.csv").string());

    //========== modelling setup ========
    // This section defines common optimizer/fit controls and CV policy.
    TrainOptions train_options{};
    train_options.max_iterations = 3000;
    train_options.tolerance = 1e-8;
    train_options.random.seed = 7;
    train_options.random.deterministic = true;

    AssessOptions assess_options{};
    assess_options.metrics = {MetricKind::Rmse, MetricKind::R2, MetricKind::Mae};

    lymph::CrossValidationOptions cv_options{};
    cv_options.plan = CVPlanKind::CrossVal;
    cv_options.folds = 5;
    cv_options.stratified = false;
    cv_options.reliability_defaults = true;
    cv_options.shuffle = true;
    cv_options.random_state = 73;
    cv_options.deterministic = true;

    //========== modelling: broad candidate sweep ========
    // This section builds a broad set of workflows spanning linear, tree,
    // ensemble, neighbor, kernel, and bayes regressors.
    std::vector<lymph::selection::Candidate> broad_candidates{};

    {
      auto wf = lymph::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::None,
          .regularization_strength = 0.0,
          .fit_intercept = true,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      broad_candidates.emplace_back("linear_ols_standard", std::move(wf));
    }
    {
      auto wf = lymph::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-8,
          .fit_intercept = true,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      broad_candidates.emplace_back("linear_ridge_1e8", std::move(wf));
    }
    {
      auto wf = lymph::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.robust_scaler();
      broad_candidates.emplace_back("linear_ridge_1e6_robust", std::move(wf));
    }
    {
      auto wf = lymph::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 5e-6,
          .fit_intercept = true,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      broad_candidates.emplace_back("linear_l2_5e6", std::move(wf));
    }

    {
      auto wf = lymph::tree_factory::decision_tree_regressor(TreeFamilyOptions{          .criterion = TreeCriterion::Mse,
          .max_depth = 12,
          .min_samples_leaf = 12,
          .min_samples_split = 30,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      broad_candidates.emplace_back("tree_depth12_leaf12", std::move(wf));
    }
    {
      auto wf = lymph::tree_factory::decision_tree_regressor(TreeFamilyOptions{          .criterion = TreeCriterion::Mse,
          .max_depth = 18,
          .min_samples_leaf = 8,
          .min_samples_split = 24,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      broad_candidates.emplace_back("tree_depth18_leaf8", std::move(wf));
    }
    {
      auto wf = lymph::tree_factory::decision_tree_regressor(TreeFamilyOptions{          .criterion = TreeCriterion::Mse,
          .max_depth = 24,
          .min_samples_leaf = 5,
          .min_samples_split = 16,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      broad_candidates.emplace_back("tree_depth24_leaf5", std::move(wf));
    }

    {
      auto wf = lymph::ensemble_factory::random_forest_regressor(EnsembleFamilyOptions{          .method = EnsembleMethod::RandomForest,
          .learners = 120,
          .subsample = 0.9,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      broad_candidates.emplace_back("forest_120_09", std::move(wf));
    }
    {
      auto wf = lymph::ensemble_factory::random_forest_regressor(EnsembleFamilyOptions{          .method = EnsembleMethod::RandomForest,
          .learners = 200,
          .subsample = 0.85,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      broad_candidates.emplace_back("forest_200_085", std::move(wf));
    }
    {
      auto wf = lymph::ensemble_factory::gradient_boost_regressor(EnsembleFamilyOptions{          .method = EnsembleMethod::GradientBoost,
          .learners = 240,
          .subsample = 0.7,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      broad_candidates.emplace_back("gboost_240_07", std::move(wf));
    }
    {
      auto wf = lymph::ensemble_factory::bagging_regressor(EnsembleFamilyOptions{          .method = EnsembleMethod::Bagging,
          .learners = 180,
          .subsample = 0.95,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      broad_candidates.emplace_back("bagging_180_095", std::move(wf));
    }

    {
      auto wf = lymph::neighbor_factory::knn_regressor(NeighborFamilyOptions{          .k = 7,
          .distance_weighted = false,
          .standardize = true,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      broad_candidates.emplace_back("knn_k7_uniform", std::move(wf));
    }
    {
      auto wf = lymph::neighbor_factory::knn_regressor(NeighborFamilyOptions{          .k = 9,
          .distance_weighted = true,
          .standardize = true,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      broad_candidates.emplace_back("knn_k9_distance", std::move(wf));
    }
    {
      auto wf = lymph::neighbor_factory::knn_regressor(NeighborFamilyOptions{          .k = 15,
          .distance_weighted = true,
          .standardize = true,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.robust_scaler();
      broad_candidates.emplace_back("knn_k15_distance_robust", std::move(wf));
    }

    {
      auto wf = lymph::kernel_factory::rbf_kernel_regressor(KernelFamilyOptions{          .kernel = KernelKind::Rbf,
          .c = 25.0,
          .gamma = 0.5,
          .epsilon = 0.0,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      broad_candidates.emplace_back("svr_rbf_c25_g05", std::move(wf));
    }
    {
      auto wf = lymph::kernel_factory::rbf_kernel_regressor(KernelFamilyOptions{          .kernel = KernelKind::Rbf,
          .c = 50.0,
          .gamma = 1.0,
          .epsilon = 0.0,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      broad_candidates.emplace_back("svr_rbf_c50_g10", std::move(wf));
    }
    {
      auto wf = lymph::kernel_factory::polynomial_kernel_regressor(KernelFamilyOptions{          .kernel = KernelKind::Polynomial,
          .c = 10.0,
          .gamma = 0.2,
          .epsilon = 0.0,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      broad_candidates.emplace_back("svr_poly_c10_g02", std::move(wf));
    }
    {
      auto wf = lymph::kernel_factory::linear_kernel_regressor(KernelFamilyOptions{          .kernel = KernelKind::Linear,
          .c = 3.0,
          .gamma = 0.0,
          .epsilon = 0.0,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      broad_candidates.emplace_back("svr_linear_c3", std::move(wf));
    }

    {
      auto wf = lymph::bayes_factory::gaussian_nb_regressor(BayesFamilyOptions{          .use_gaussian = true,
          .smoothing = 1e-9,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      broad_candidates.emplace_back("bayes_gaussian_1e9", std::move(wf));
    }
    {
      auto wf = lymph::bayes_factory::gaussian_nb_regressor(BayesFamilyOptions{          .use_gaussian = true,
          .smoothing = 1e-6,
      });
      wf.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      broad_candidates.emplace_back("bayes_gaussian_1e6", std::move(wf));
    }

    //========== modelling: cross-validation on broad sweep ========
    // This section ranks all broad candidates by CV RMSE.
    auto broad_tune = lymph::selection::compare(
        std::move(broad_candidates),
        train_x,
        train_y,
        cv_options,
        train_options,
        assess_options,
        MetricKind::Rmse,
        TuneObjectivePolicy::Minimize);

    std::vector<std::size_t> broad_order;
    broad_order.reserve(broad_tune.report.ranked_indices.size());
    for (const auto index : broad_tune.report.ranked_indices) {
      broad_order.push_back(static_cast<std::size_t>(index));
    }

    std::cout << "\n========== broad cv ranking (rmse, lower is better) ========\n";
    for (std::size_t rank = 0; rank < broad_order.size(); ++rank) {
      const auto& trial = broad_tune.report.trials[broad_order[rank]];
      std::cout << "rank=" << (rank + 1)
                << " candidate=" << trial.name
                << " objective_rmse=" << trial.objective
                << " cv_rmse=" << trial.cv.mean.rmse
                << " cv_r2=" << trial.cv.mean.r2
                << " cv_mae=" << trial.cv.mean.mae
                << "\n";
    }

    //========== modelling: top-three extraction ========
    // This section extracts the top three model families from broad sweep.
    const std::string top1_name = broad_tune.report.trials[broad_order[0]].name;
    const std::string top2_name = broad_tune.report.trials[broad_order[1]].name;
    const std::string top3_name = broad_tune.report.trials[broad_order[2]].name;

    std::cout << "\n========== top three candidates ========\n";
    std::cout << "top1=" << top1_name << "\n";
    std::cout << "top2=" << top2_name << "\n";
    std::cout << "top3=" << top3_name << "\n";

    bool use_linear = false;
    bool use_tree = false;
    bool use_ensemble = false;
    bool use_neighbor = false;
    bool use_kernel = false;
    bool use_bayes = false;

    {
      const std::vector<std::string> top_names = {top1_name, top2_name, top3_name};
      for (const auto& name : top_names) {
        if (name.find("linear") != std::string::npos) use_linear = true;
        if (name.find("tree") != std::string::npos) use_tree = true;
        if (name.find("forest") != std::string::npos || name.find("bagging") != std::string::npos || name.find("gboost") != std::string::npos) {
          use_ensemble = true;
        }
        if (name.find("knn") != std::string::npos || name.find("neighbor") != std::string::npos) use_neighbor = true;
        if (name.find("svr") != std::string::npos || name.find("kernel") != std::string::npos) use_kernel = true;
        if (name.find("bayes") != std::string::npos) use_bayes = true;
      }
    }

    //========== tuning: targeted search on top-three families ========
    // This section creates a denser candidate grid only around families
    // that appeared in the top-three broad sweep.
    std::vector<lymph::selection::Candidate> tuned_candidates{};

    if (use_linear) {
      auto wf1 = lymph::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-9,
          .fit_intercept = true,
      });
      wf1.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf1.standard_scaler();
      tuned_candidates.emplace_back("tuned_linear_l2_1e9", std::move(wf1));

      auto wf2 = lymph::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-7,
          .fit_intercept = true,
      });
      wf2.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf2.standard_scaler();
      tuned_candidates.emplace_back("tuned_linear_l2_1e7", std::move(wf2));

      auto wf3 = lymph::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      });
      wf3.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf3.standard_scaler();
      tuned_candidates.emplace_back("tuned_linear_l2_1e6", std::move(wf3));

      auto wf4 = lymph::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 5e-7,
          .fit_intercept = true,
      });
      wf4.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf4.standard_scaler();
      tuned_candidates.emplace_back("tuned_linear_l2_5e7", std::move(wf4));
    }

    if (use_tree) {
      auto wf1 = lymph::tree_factory::decision_tree_regressor(TreeFamilyOptions{          .criterion = TreeCriterion::Mse,
          .max_depth = 16,
          .min_samples_leaf = 6,
          .min_samples_split = 20,
      });
      wf1.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("tuned_tree_depth16_leaf6", std::move(wf1));

      auto wf2 = lymph::tree_factory::decision_tree_regressor(TreeFamilyOptions{          .criterion = TreeCriterion::Mse,
          .max_depth = 22,
          .min_samples_leaf = 4,
          .min_samples_split = 14,
      });
      wf2.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("tuned_tree_depth22_leaf4", std::move(wf2));

      auto wf3 = lymph::tree_factory::decision_tree_regressor(TreeFamilyOptions{          .criterion = TreeCriterion::Mse,
          .max_depth = 18,
          .min_samples_leaf = 5,
          .min_samples_split = 12,
      });
      wf3.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("tuned_tree_depth18_leaf5", std::move(wf3));
    }

    if (use_ensemble) {
      auto wf1 = lymph::ensemble_factory::random_forest_regressor(EnsembleFamilyOptions{          .method = EnsembleMethod::RandomForest,
          .learners = 180,
          .subsample = 0.9,
      });
      wf1.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("tuned_forest_180_09", std::move(wf1));

      auto wf2 = lymph::ensemble_factory::random_forest_regressor(EnsembleFamilyOptions{          .method = EnsembleMethod::RandomForest,
          .learners = 260,
          .subsample = 0.85,
      });
      wf2.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("tuned_forest_260_085", std::move(wf2));

      auto wf3 = lymph::ensemble_factory::gradient_boost_regressor(EnsembleFamilyOptions{          .method = EnsembleMethod::GradientBoost,
          .learners = 320,
          .subsample = 0.7,
      });
      wf3.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("tuned_gboost_320_07", std::move(wf3));

      auto wf4 = lymph::ensemble_factory::bagging_regressor(EnsembleFamilyOptions{          .method = EnsembleMethod::Bagging,
          .learners = 280,
          .subsample = 0.92,
      });
      wf4.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("tuned_bagging_280_092", std::move(wf4));
    }

    if (use_neighbor) {
      auto wf1 = lymph::neighbor_factory::knn_regressor(NeighborFamilyOptions{          .k = 8,
          .distance_weighted = true,
          .standardize = true,
      });
      wf1.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf1.standard_scaler();
      tuned_candidates.emplace_back("tuned_knn_k8_distance", std::move(wf1));

      auto wf2 = lymph::neighbor_factory::knn_regressor(NeighborFamilyOptions{          .k = 12,
          .distance_weighted = true,
          .standardize = true,
      });
      wf2.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf2.standard_scaler();
      tuned_candidates.emplace_back("tuned_knn_k12_distance", std::move(wf2));

      auto wf3 = lymph::neighbor_factory::knn_regressor(NeighborFamilyOptions{          .k = 16,
          .distance_weighted = true,
          .standardize = true,
      });
      wf3.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf3.robust_scaler();
      tuned_candidates.emplace_back("tuned_knn_k16_distance_robust", std::move(wf3));
    }

    if (use_kernel) {
      auto wf1 = lymph::kernel_factory::rbf_kernel_regressor(KernelFamilyOptions{          .kernel = KernelKind::Rbf,
          .c = 40.0,
          .gamma = 0.6,
          .epsilon = 0.0,
      });
      wf1.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf1.standard_scaler();
      tuned_candidates.emplace_back("tuned_svr_rbf_c40_g06", std::move(wf1));

      auto wf2 = lymph::kernel_factory::rbf_kernel_regressor(KernelFamilyOptions{          .kernel = KernelKind::Rbf,
          .c = 80.0,
          .gamma = 1.2,
          .epsilon = 0.0,
      });
      wf2.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf2.standard_scaler();
      tuned_candidates.emplace_back("tuned_svr_rbf_c80_g12", std::move(wf2));

      auto wf3 = lymph::kernel_factory::polynomial_kernel_regressor(KernelFamilyOptions{          .kernel = KernelKind::Polynomial,
          .c = 20.0,
          .gamma = 0.3,
          .epsilon = 0.0,
      });
      wf3.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf3.standard_scaler();
      tuned_candidates.emplace_back("tuned_svr_poly_c20_g03", std::move(wf3));

      auto wf4 = lymph::kernel_factory::linear_kernel_regressor(KernelFamilyOptions{          .kernel = KernelKind::Linear,
          .c = 5.0,
          .gamma = 0.0,
          .epsilon = 0.0,
      });
      wf4.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf4.standard_scaler();
      tuned_candidates.emplace_back("tuned_svr_linear_c5", std::move(wf4));
    }

    if (use_bayes) {
      auto wf1 = lymph::bayes_factory::gaussian_nb_regressor(BayesFamilyOptions{          .use_gaussian = true,
          .smoothing = 1e-10,
      });
      wf1.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("tuned_bayes_1e10", std::move(wf1));

      auto wf2 = lymph::bayes_factory::gaussian_nb_regressor(BayesFamilyOptions{          .use_gaussian = true,
          .smoothing = 1e-8,
      });
      wf2.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("tuned_bayes_1e8", std::move(wf2));
    }

    if (tuned_candidates.size() < 3) {
      auto wf1 = lymph::ensemble_factory::random_forest_regressor(EnsembleFamilyOptions{          .method = EnsembleMethod::RandomForest,
          .learners = 220,
          .subsample = 0.9,
      });
      wf1.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      tuned_candidates.emplace_back("fallback_forest_220_09", std::move(wf1));

      auto wf2 = lymph::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-7,
          .fit_intercept = true,
      });
      wf2.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf2.standard_scaler();
      tuned_candidates.emplace_back("fallback_linear_l2", std::move(wf2));

      auto wf3 = lymph::neighbor_factory::knn_regressor(NeighborFamilyOptions{          .k = 10,
          .distance_weighted = true,
          .standardize = true,
      });
      wf3.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf3.standard_scaler();
      tuned_candidates.emplace_back("fallback_knn_k10", std::move(wf3));
    }

    auto tuned_final = lymph::selection::compare(
        std::move(tuned_candidates),
        train_x,
        train_y,
        cv_options,
        train_options,
        assess_options,
        MetricKind::Rmse,
        TuneObjectivePolicy::Minimize);

    std::vector<std::size_t> tuned_order;
    tuned_order.reserve(tuned_final.report.ranked_indices.size());
    for (const auto index : tuned_final.report.ranked_indices) {
      tuned_order.push_back(static_cast<std::size_t>(index));
    }

    std::cout << "\n========== tuned cv ranking (rmse, lower is better) ========\n";
    for (std::size_t rank = 0; rank < tuned_order.size(); ++rank) {
      const auto& trial = tuned_final.report.trials[tuned_order[rank]];
      std::cout << "rank=" << (rank + 1)
                << " candidate=" << trial.name
                << " objective_rmse=" << trial.objective
                << " cv_rmse=" << trial.cv.mean.rmse
                << " cv_r2=" << trial.cv.mean.r2
                << " cv_mae=" << trial.cv.mean.mae
                << "\n";
    }

    //========== final model ========
    // This section evaluates the fitted best workflow returned by tune,
    // then saves/loads for reproducibility.
    auto final_workflow = std::move(tuned_final.best());

    const auto final_metrics = final_workflow.assess(test_x, test_y, assess_options);
    std::cout << "\n========== final holdout metrics ========\n";
    std::cout << "rmse=" << final_metrics.rmse << "\n";
    std::cout << "r2=" << final_metrics.r2 << "\n";
    std::cout << "mae=" << final_metrics.mae << "\n";

    const Series test_pred = final_workflow.infer(test_x);
    const Series test_pred_scores = final_workflow.infer_scores(test_x);

    const fs::path final_model_path = model_dir / "lymph_hour_end_to_end_final.bin";
    final_workflow.save(final_model_path.string(), train_x, train_y, train_options);

    const auto restored_workflow = lymph::WorkflowModel::load(final_model_path.string());
    const Series restored_pred = restored_workflow.infer(test_x);
    const double restored_max_abs_diff = lymph::max_abs_diff(test_pred, restored_pred);

    std::cout << "\n========== persistence check ========\n";
    std::cout << "model_path=" << final_model_path << "\n";
    std::cout << "restored_max_abs_diff=" << restored_max_abs_diff << "\n";

    //========== final diagnostics artifacts ========
    // This section exports prediction diagnostics for direct inspection.
    const Series y_true_cpu = test_y.to(pluss::pCPU, ScalarType::Float64).reshape({test_y.numel()});
    const Series y_pred_cpu = test_pred.to(pluss::pCPU, ScalarType::Float64).reshape({test_pred.numel()});
    const Series y_score_cpu = test_pred_scores.to(pluss::pCPU, ScalarType::Float64).reshape({test_pred_scores.numel()});
    const Series residual_cpu = y_true_cpu.sub(y_pred_cpu);
    const Series abs_error_cpu = residual_cpu.abs();
    const Series sq_error_cpu = residual_cpu.square();

    DataFrame pred_frame({
        {"y_true", Thread(y_true_cpu)},
        {"y_pred", Thread(y_pred_cpu)},
        {"y_score", Thread(y_score_cpu)},
        {"residual", Thread(residual_cpu)},
        {"abs_error", Thread(abs_error_cpu)},
        {"sq_error", Thread(sq_error_cpu)},
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

    const DataFrame hour_error = pred_frame.group({.dropnull = true})
                                     .by({"y_true"})
                                     .agg({
                                         {"mean_abs_error", "abs_error", DataFrame::GroupOp::Mean},
                                         {"count", "abs_error", DataFrame::GroupOp::Count},
                                     });
    hour_error.save_csv((out_dir / "final_abs_error_by_y_true.csv").string());

    //========== run summary ========
    // This section writes compact run metadata for quick reproducibility checks.
    DataFrame run_summary({
        {"n_rows_loaded", Thread(pluss::make_series(std::vector<double>{static_cast<double>(frame.ncases())}, {1}))},
        {"n_features_used", Thread(pluss::make_series(std::vector<double>{static_cast<double>(model_features.size())}, {1}))},
        {"holdout_train_rows", Thread(pluss::make_series(std::vector<double>{static_cast<double>(train_x.ncases())}, {1}))},
        {"holdout_test_rows", Thread(pluss::make_series(std::vector<double>{static_cast<double>(test_x.ncases())}, {1}))},
        {"broad_candidates", Thread(pluss::make_series(std::vector<double>{static_cast<double>(broad_tune.report.trials.size())}, {1}))},
        {"tuned_candidates", Thread(pluss::make_series(std::vector<double>{static_cast<double>(tuned_final.report.trials.size())}, {1}))},
        {"final_rmse", Thread(final_metrics.rmse)},
        {"final_r2", Thread(final_metrics.r2)},
        {"final_mae", Thread(final_metrics.mae)},
        {"restored_max_abs_diff", Thread(pluss::make_series(std::vector<double>{restored_max_abs_diff}, {1}))},
    });
    run_summary.save_csv((out_dir / "run_summary.csv").string());

    std::cout << "\n========== completed ========\n";
    std::cout << "saved_outputs=" << out_dir << "\n";
    std::cout << "saved_model=" << final_model_path << "\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "lymph_end_to_end_hour_workflow_showcase failed: " << ex.what() << "\n";
    return 1;
  }
}
