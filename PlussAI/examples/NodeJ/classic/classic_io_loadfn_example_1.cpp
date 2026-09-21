#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();
  flow.add_standard_scaler();

  const cls::Series x = pluss::make_series(
      {10.0, 12.0, 14.0, 16.0, 100.0, 102.0, 104.0, 106.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  flow.learn(batch);

  const cls::Series original_scores = flow.infer(batch.features).value();

  const std::string path = "/tmp/pluss_classic_io_load_example_1.bundle";
  cls::io::save(path, flow, batch);

  const cls::Flow restored = cls::io::load_and_restore(path).move_value();
  const cls::Series restored_scores = restored.infer(batch.features).value();

  const double max_abs_diff =
      (original_scores - restored_scores).abs().amax(0, false).item<double>();
  std::cout << "restored.trained() = " << std::boolalpha << restored.trained() << "\n";
  std::cout << "max abs diff between predictions = " << max_abs_diff << "\n";

  return 0;
}
