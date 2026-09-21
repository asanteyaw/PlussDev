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
  // flow.learn(batch) never called.

  const std::string path = "/tmp/pluss_classic_io_load_example_2.bundle";
  cls::io::save(path, flow, batch);
  std::cout << "saved flow.trained() = " << std::boolalpha << flow.trained() << "\n";

  const cls::Flow restored = cls::io::load_and_restore(path).move_value();
  std::cout << "restored.trained()   = " << restored.trained() << "\n";
  std::cout << "restored infer ok    = " << restored.infer(batch.features).ok() << "\n";

  return 0;
}
