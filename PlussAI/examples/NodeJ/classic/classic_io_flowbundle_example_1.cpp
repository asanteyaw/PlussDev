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

  const std::string path = "/tmp/pluss_classic_io_flowbundle_example_1.bundle";
  cls::io::save(path, flow, batch);

  const cls::io::FlowBundle bundle = cls::io::load(path).value();
  std::cout << "producer       = " << bundle.metadata.producer << "\n";
  std::cout << "schema_version = " << bundle.metadata.schema_version << "\n";
  std::cout << "model.family   = " << static_cast<int>(bundle.model.family) << "\n";
  std::cout << "trained        = " << std::boolalpha << bundle.trained << "\n";
  std::cout << "has_snapshot   = " << bundle.has_snapshot << "\n";
  std::cout << "transforms.size() = " << bundle.transforms.size() << "\n";

  return 0;
}
