#include <pluss/ml.h>
#include <pluss/mic.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

[[nodiscard]] fs::path default_data_path() {
  return fs::path(PLUSSMIC_SOURCE_DIR) / "data" / "hour.csv";
}

[[nodiscard]] fs::path default_output_dir() {
  return fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "outputs" / "bike_sharing_eda_dfplot";
}

int main(int argc, char** argv) {
  using DataFrame = pluss::table::DataFrame;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::Thread;
  using namespace pluss::ai::nodej;
  namespace lymph = pluss::ai::lymph;

  try {
    fs::path data_path = default_data_path();
    fs::path out_dir = default_output_dir();
    if (argc > 1 && argv[1] != nullptr && std::string(argv[1]).size() > 0) data_path = fs::path(argv[1]);
    if (argc > 2 && argv[2] != nullptr && std::string(argv[2]).size() > 0) out_dir = fs::path(argv[2]);
    const fs::path model_dir = fs::path(PLUSSAI_SOURCE_DIR) / "examples" / "NodeJ" / "trained_models";
    fs::create_directories(out_dir);
    fs::create_directories(model_dir);

    auto frame = DataFrame::load_csv(data_path.string())
                     .rename("instant", "rec_id")
                     .rename("dteday", "date")
                     .rename("holiday", "is_holiday")
                     .rename("workingday", "is_workingday")
                     .rename("weathersit", "weather_condition")
                     .rename("hum", "humidity")
                     .rename("mnth", "month")
                     .rename("cnt", "total_count")
                     .rename("hr", "hour")
                     .rename("yr", "year");
  frame.set("date") = frame.features("date").cast().timeline();
  frame = frame.astype({
                         {"rec_id", ScalarType::Float64},
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

    frame.plot()
        .hist("total_count", 64)
        .headline("Bike demand distribution")
        .save_pdf((out_dir / "bike_demand_hist.pdf").string());
    frame.plot()
        .scatter("temp", "total_count")
        .headline("Bike demand vs temperature")
        .save_pdf((out_dir / "bike_demand_vs_temp.pdf").string());

    const auto split = lymph::holdout_split(frame, lymph::HoldoutSplitOptions{
                                                     .test_size = 0.2,
                                                     .random_state = 42,
                                                     .deterministic = true,
                                                     .shuffle = true,
                                                 });
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

    auto linear_workflow = lymph::linear_factory::linear_regression(LinearFamilyOptions{
        .objective = LinearObjective::LeastSquares,
        .regularization = RegularizationKind::None,
        .regularization_strength = 0.0,
        .fit_intercept = true,
    });
    linear_workflow.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
    linear_workflow.standard_scaler();
    linear_workflow.learn_(train_x, train_y, train_options);
    const auto linear_metrics = linear_workflow.assess(test_x, test_y, assess_options);
    std::cout << "linear test metrics [rmse, r2, mae]: " << linear_metrics.rmse << " " << linear_metrics.r2 << " "
              << linear_metrics.mae << "\n";
    std::cout << "linear params: intercept=" << linear_workflow.intercept << " coeff=" << linear_workflow.coeff << "\n";

    const Series linear_pred_series = linear_workflow.infer(test_x);
    const fs::path linear_model_path = model_dir / "bike_sharing_linear.bin";
    linear_workflow.save(linear_model_path.string(), train_x, train_y, train_options);
    const auto linear_restored = lymph::WorkflowModel::load(linear_model_path.string());
    const Series linear_pred_restored_series = linear_restored.infer(test_x);
    const Series y_true = test_y.to(pluss::pCPU, ScalarType::Float64).reshape({test_y.numel()});

    const Series linear_pred = linear_pred_series.to(pluss::pCPU, ScalarType::Float64).reshape({linear_pred_series.numel()});
    const double linear_max_abs_diff = lymph::max_abs_diff(linear_pred_series, linear_pred_restored_series);
    std::cout << "linear restored prediction max_abs_diff: " << linear_max_abs_diff << "\n";

    DataFrame linear_frame({
        {"y_true", Thread(y_true)},
        {"y_pred", Thread(linear_pred)},
        {"residual", Thread(y_true.sub(linear_pred))},
    });
    linear_frame.save_csv((out_dir / "bike_sharing_test_predictions_linear.csv").string());
    linear_frame.plot()
        .scatter("y_true", "y_pred")
        .headline("Observed vs predicted bike demand (linear)")
        .save_pdf((out_dir / "bike_pred_vs_true_linear.pdf").string());
    linear_frame.plot()
        .hist("residual", 80)
        .headline("Residual distribution (linear)")
        .save_pdf((out_dir / "bike_residual_hist_linear.pdf").string());
    std::cout << "saved model (linear): " << linear_model_path << "\n";

    auto tree_workflow = lymph::tree_factory::decision_tree_regressor(TreeFamilyOptions{
        .criterion = TreeCriterion::Mse,
        .max_depth = 18,
        .min_samples_leaf = 8,
        .min_samples_split = 24,
    });
    tree_workflow.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
    tree_workflow.learn_(train_x, train_y, train_options);
    const auto tree_metrics = tree_workflow.assess(test_x, test_y, assess_options);
    std::cout << "tree test metrics [rmse, r2, mae]: " << tree_metrics.rmse << " " << tree_metrics.r2 << " "
              << tree_metrics.mae << "\n";

    const Series tree_pred_series = tree_workflow.infer(test_x);
    const fs::path tree_model_path = model_dir / "bike_sharing_tree.bin";
    tree_workflow.save(tree_model_path.string(), train_x, train_y, train_options);
    const auto tree_restored = lymph::WorkflowModel::load(tree_model_path.string());
    const Series tree_pred_restored_series = tree_restored.infer(test_x);

    const Series tree_pred = tree_pred_series.to(pluss::pCPU, ScalarType::Float64).reshape({tree_pred_series.numel()});
    const double tree_max_abs_diff = lymph::max_abs_diff(tree_pred_series, tree_pred_restored_series);
    std::cout << "tree restored prediction max_abs_diff: " << tree_max_abs_diff << "\n";


    // New Model
    DataFrame tree_frame({
        {"y_true", Thread(y_true)},
        {"y_pred", Thread(tree_pred)},
        {"residual", Thread(y_true.sub(tree_pred))},
    });
    tree_frame.save_csv((out_dir / "bike_sharing_test_predictions_tree.csv").string());
    tree_frame.plot()
        .scatter("y_true", "y_pred")
        .headline("Observed vs predicted bike demand (tree)")
        .save_pdf((out_dir / "bike_pred_vs_true_tree.pdf").string());
    tree_frame.plot()
        .hist("residual", 80)
        .headline("Residual distribution (tree)")
        .save_pdf((out_dir / "bike_residual_hist_tree.pdf").string());
    std::cout << "saved model (tree): " << tree_model_path << "\n";

    std::cout << "saved outputs under: " << out_dir << "\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "bike_sharing_end_to_end failed: " << ex.what() << "\n";
    return 1;
  }
}
