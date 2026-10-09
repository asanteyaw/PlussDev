#include <pluss/ml.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
  using DataFrame = pluss::table::DataFrame;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::Slate;
  using namespace pluss::ai::nodej;
  namespace nja = pluss::ai::lymph;

  try {
    fs::path data_path = fs::path(PLUSSMIC_SOURCE_DIR) / "data" / "hour.csv";
    fs::path out_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "outputs" / "nodej_workflow_tuning_demo";
    fs::path model_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "NodeJ" / "trained_models";
    std::int64_t ncases = 6000;
    if (argc > 1 && argv[1] != nullptr && std::string(argv[1]).size() > 0) data_path = fs::path(argv[1]);
    if (argc > 2 && argv[2] != nullptr && std::string(argv[2]).size() > 0) out_dir = fs::path(argv[2]);
    if (argc > 3 && argv[3] != nullptr && std::string(argv[3]).size() > 0) ncases = std::stoll(argv[3]);
    fs::create_directories(out_dir);
    fs::create_directories(model_dir);

    const DataFrame raw = DataFrame::load_csv(data_path.string(), {{"ncases", ncases}});
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


    const auto split = nja::selection::validate::holdout()
                           .test_size(0.2)
                           .shuffle(true)
                           .seed(42)
                           .split(frame);
    const DataFrame train_x = split.train_set.features(
        {"season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition", "temp", "atemp", "humidity", "windspeed"});
    const DataFrame test_x = split.test_set.features(
        {"season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition", "temp", "atemp", "humidity", "windspeed"});
    const Series train_y = split.train_set.features("total_count").as_series();
    const Series test_y = split.test_set.features("total_count").as_series();

    TrainOptions train_options{};
    train_options.max_iterations = 2500;
    train_options.tolerance = 1e-8;
    train_options.random.seed = 7;
    train_options.random.deterministic = true;

    AssessOptions assess_options{};
    assess_options.metrics = {MetricKind::Rmse, MetricKind::R2, MetricKind::Mae};

    nja::selection::CrossValOptions cv_options{};
    cv_options.plan = CVPlanKind::CrossVal;
    cv_options.folds = 5;
    cv_options.shuffle = true;
    cv_options.random_state = 73;
    cv_options.deterministic = true;

    std::vector<nja::selection::Candidate> candidates{};
    {
      nja::models::Model wf = nja::models::linear_regression(LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      });
      wf.simple_imputer(nja::prep::SimpleImputerOptions{.strategy = nja::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      candidates.emplace_back("linear_ridge", wf);
    }
    {
      nja::models::Model wf = nja::models::decision_tree_regressor(TreeFamilyOptions{
          .criterion = TreeCriterion::Mse,
          .max_depth = 18,
          .min_samples_leaf = 8,
          .min_samples_split = 24,
      });
      wf.simple_imputer(nja::prep::SimpleImputerOptions{.strategy = nja::prep::SimpleImputerStrategy::Median});
      candidates.emplace_back("decision_tree", wf);
    }
    {
      nja::models::Model wf = nja::models::random_forest_regressor(EnsembleFamilyOptions{
          .method = EnsembleMethod::RandomForest,
          .learners = 140,
          .subsample = 0.9,
      });
      wf.simple_imputer(nja::prep::SimpleImputerOptions{.strategy = nja::prep::SimpleImputerStrategy::Median});
      candidates.emplace_back("random_forest", wf);
    }
    {
      nja::models::Model wf = nja::models::knn_regressor(NeighborFamilyOptions{
          .k = 9,
          .distance_weighted = true,
          .standardize = true,
      });
      wf.simple_imputer(nja::prep::SimpleImputerOptions{.strategy = nja::prep::SimpleImputerStrategy::Median});
      candidates.emplace_back("knn_distance", wf);
    }

    auto tuned = nja::selection::compare(
        std::move(candidates),
        train_x,
        train_y,
        cv_options,
        train_options,
        assess_options,
        MetricKind::Rmse,
        TuneObjectivePolicy::Minimize);

    std::cout << "CV objective metric: rmse\n";
    for (const auto& trial : tuned.report.trials) {
      std::cout << "candidate=" << trial.name << " objective=" << trial.objective
                << " cv_rmse=" << trial.cv.mean.rmse
                << " cv_r2=" << trial.cv.mean.r2
                << " cv_mae=" << trial.cv.mean.mae << "\n";
    }
    std::cout << "best candidate: " << tuned.report.best_name << "\n";

    auto best = std::move(tuned.best());
    const auto test_metrics = best.assess(test_x, test_y, assess_options);
    std::cout << "test metrics [rmse, r2, mae]: " << test_metrics.rmse << " " << test_metrics.r2 << " " << test_metrics.mae << "\n";

    const Series pred = best.infer(test_x);
    const fs::path best_model_path = model_dir / "hour_best_workflow.bin";
    best.save(best_model_path.string(), train_x, train_y, train_options);
    auto restored = nja::models::Model::load(best_model_path.string());
    const Series pred_restored = restored.infer(test_x);
    const double restored_max_abs_diff = nja::max_abs_diff(pred, pred_restored);
    std::cout << "restored prediction max_abs_diff: " << restored_max_abs_diff << "\n";

    const Series y_true = test_y.to(pluss::pCPU, ScalarType::Float64).reshape({test_y.numel()});
    const Series y_pred = pred.to(pluss::pCPU, ScalarType::Float64).reshape({pred.numel()});
    DataFrame pred_frame({
        {"y_true", Slate(y_true)},
        {"y_pred", Slate(y_pred)},
        {"residual", Slate(y_true.sub(y_pred))},
    });
    pred_frame.save_csv((out_dir / "hour_workflow_tuning_predictions.csv").string());
    pred_frame.plot()
        .scatter("y_true", "y_pred")
        .headline("Observed vs predicted bike demand (best workflow)")
        .save_pdf((out_dir / "hour_workflow_tuning_pred_vs_true.pdf").string());

    std::cout << "saved best model: " << best_model_path << "\n";
    std::cout << "saved outputs under: " << out_dir << "\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "nodej_workflow_tuning_demo failed: " << ex.what() << "\n";
    return 1;
  }
}
