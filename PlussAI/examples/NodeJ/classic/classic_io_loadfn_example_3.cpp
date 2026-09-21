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

  const std::string path = "/tmp/pluss_classic_io_load_example_3.bundle";
  cls::io::save(path, flow, batch);

  const cls::io::FlowBundle bundle = cls::io::load(path).value();
  const cls::Flow via_two_steps = cls::io::restore(bundle).move_value();
  const cls::Flow via_one_call = cls::io::load_and_restore(path).move_value();

  const cls::Series scores_a = via_two_steps.infer(batch.features).value();
  const cls::Series scores_b = via_one_call.infer(batch.features).value();
  const double max_abs_diff = (scores_a - scores_b).abs().amax(0, false).item<double>();
  std::cout << "max abs diff between the two paths = " << max_abs_diff << "\n";

  return 0;
}
