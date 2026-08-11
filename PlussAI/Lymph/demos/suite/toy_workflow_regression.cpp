#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const Series x = pluss::series::linspace(-3.0, 3.0, 96, ScalarType::Float64);
  const Series wave = x.sin().add(x.mul(0.7).cos().mul(0.35));
  const Series trend = x.mul(0.22);
  const Series target = wave.add(trend).add(2.0);

  const DataFrame raw({
      {"x", Thread(x)},
      {"wave", Thread(wave)},
      {"trend", Thread(trend)},
      {"target", Thread(target)},
  });

  lymph::prep::SimpleImputerOptions fill{};
  fill.strategy = lymph::prep::SimpleImputerStrategy::Median;

  lymph::WorkflowOptions flow{};
  flow.workers = 2;

  lymph::Payload data = lymph::payload::make(raw)
      .features({"x", "wave", "trend"})
      .label("target");

  lymph::Workflow design = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"x", "wave", "trend"}, {
              {"fill", lymph::stage::simple_imputer(fill)},
              {"scale", lymph::stage::standard_scaler()},
              {"shape", lymph::stage::power_map()},
          }),
      },
      lymph::models::random_forest_regressor({.learners = 31, .subsample = 1.0}),
      flow);

  lymph::TrainOptions train{};
  train.random.seed = 20260811ULL;
  train.random.deterministic = true;

  design.learn_(data, train);
  const lymph::MetricReport report = design.assess(data);
  const Series pred = design.infer(data);

  const std::filesystem::path bundle =
      std::filesystem::temp_directory_path() / "plussai_toy_workflow_regression.bundle";
  design.save(bundle.string(), data, train);
  std::filesystem::remove(bundle);
  std::filesystem::remove(bundle.string() + ".policy");

  std::cout << "toy regression rmse: " << report.rmse.item<double>() << "\n";
  std::cout << "predictions: " << pred.numel() << "\n";
  std::cout << "workflow branches: " << design.branch_count() << "\n";
  return 0;
}
