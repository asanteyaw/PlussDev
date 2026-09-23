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
  const cls::Status learned = model.learn(x, y);
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "trained()      = " << model.trained() << "\n";
  std::cout << "feature_count() = " << model.feature_count() << "\n";

  std::cout << "infer_scores = " << model.infer_scores(x).value() << "\n";
  std::cout << "infer        = " << model.infer(x).value() << "\n";
  std::cout << "assess (Accuracy) = " << model.assess(x, y).value() << "\n";

  return 0;
}
