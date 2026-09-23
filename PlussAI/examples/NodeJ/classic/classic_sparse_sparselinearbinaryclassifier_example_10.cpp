#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearBinaryClassifier model;
  model.learn(x, y);

  cls::AssessOptions unsupported{};
  unsupported.metrics = {cls::MetricKind::RocAuc};
  const cls::StatusOr<pluss::Series> rejected = model.assess(x, y, unsupported);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "status code = " << static_cast<int>(rejected.status().code) << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
