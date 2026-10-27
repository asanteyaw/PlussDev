#include <pluss/ai/ml.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

int main() {
  using namespace pluss::ai::nodej;
  namespace nio = pluss::ai::nodej::io;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const Series x = pluss::make_series({
                                          -2.0, -1.0,
                                          -1.0, -0.5,
                                           1.0,  0.5,
                                           2.0,  1.0,
                                          -1.5, -0.6,
                                           1.5,  0.7},
                                      {6, 2},
                                      cfg);
  const Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0, 0.0, 1.0}, {6}, cfg);
  const LabeledBatch batch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };

  ModelSpec spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
  };

  auto flow_or = make_flow(spec);
  if (!flow_or.ok()) {
    std::cerr << "make_flow failed: " << flow_or.status().message << "\n";
    return 1;
  }
  Flow flow = flow_or.move_value();
  if (!flow.add_standard_scaler().ok()) {
    std::cerr << "add_standard_scaler failed\n";
    return 1;
  }
  TrainOptions train{};
  train.max_iterations = 2500;
  train.tolerance = 1e-8;
  train.random.seed = 77;
  train.random.deterministic = true;
  if (!flow.learn(batch, train).ok()) {
    std::cerr << "flow learn failed\n";
    return 1;
  }

  auto pred_before_or = flow.infer(batch.features);
  if (!pred_before_or.ok()) {
    std::cerr << "flow infer before save failed: " << pred_before_or.status().message << "\n";
    return 1;
  }
  const std::string model_dir = "/Users/yawasante/software/cxx/PlussDev/PlussAI/examples/NodeJ/trained_models";
  std::error_code mkdir_ec;
  std::filesystem::create_directories(model_dir, mkdir_ec);
  if (mkdir_ec) {
    std::cerr << "failed to create model directory: " << model_dir << " (" << mkdir_ec.message() << ")\n";
    return 1;
  }
  const std::string bundle_path = model_dir + "/nodej_flow_bundle_demo.bin";
  auto save_status = nio::save(bundle_path, flow, batch, train);
  if (!save_status.ok()) {
    std::cerr << "save failed: " << save_status.message << "\n";
    return 1;
  }
  std::cout << "saved flow bundle to: " << bundle_path << "\n";

  auto restored_or = nio::load_and_restore(bundle_path);
  if (!restored_or.ok()) {
    std::cerr << "load_and_restore failed: " << restored_or.status().message << "\n";
    return 1;
  }
  auto pred_after_or = restored_or.value().infer(batch.features);
  if (!pred_after_or.ok()) {
    std::cerr << "restored flow infer failed: " << pred_after_or.status().message << "\n";
    return 1;
  }

  const auto before = pred_before_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  const auto after = pred_after_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  bool same = before.size() == after.size();
  for (std::size_t i = 0; same && i < before.size(); ++i) {
    same = std::abs(before[i] - after[i]) < 1e-9;
  }
  std::cout << "flow bundle roundtrip exact prediction parity: " << (same ? "true" : "false") << "\n";
  std::cout << "nodej_flow_bundle_demo complete\n";
  return same ? 0 : 2;
}
