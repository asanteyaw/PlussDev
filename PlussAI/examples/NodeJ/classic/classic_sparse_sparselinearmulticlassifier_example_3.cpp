#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      6, 3, {0, 1, 2, 3, 4, 5}, {0, 0, 1, 1, 2, 2}, {1.0, 2.0, 1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearMultiClassifier model;
  model.learn(x, y);

  const pluss::Series unseen_class = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 3.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  const cls::StatusOr<pluss::Series> rejected = model.assess(x, unseen_class);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
