#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const std::vector<cls::ModelSpec> candidates{};
  const cls::StatusOr<cls::TuneReport> report = cls::sweep(candidates, batch);
  std::cout << "ok = " << std::boolalpha << report.ok() << "\n";
  std::cout << "message = \"" << report.status().message << "\"\n";

  return 0;
}
