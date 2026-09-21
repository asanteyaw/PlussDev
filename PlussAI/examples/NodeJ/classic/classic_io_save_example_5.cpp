#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // empty -- no model attached
  flow.add_standard_scaler();

  const cls::Series x = pluss::make_series(
      {10.0, 12.0, 14.0, 16.0, 100.0, 102.0, 104.0, 106.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  const cls::Status learned = flow.learn(batch);
  std::cout << "learned ok = " << std::boolalpha << learned.ok() << "\n";

  const cls::Status saved = cls::io::save("/tmp/pluss_classic_io_save_example_5.bundle", flow, batch);
  std::cout << "saved ok = " << saved.ok() << "\n";
  std::cout << "message = \"" << saved.message << "\"\n";

  return 0;
}
