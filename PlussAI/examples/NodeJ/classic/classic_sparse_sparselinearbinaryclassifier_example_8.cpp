#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::TrainOptions zero_iters{};
  zero_iters.max_iterations = 0;
  cls::SparseLinearBinaryClassifier a;
  std::cout << "max_iterations=0 ok = " << std::boolalpha << a.learn(x, y, zero_iters).ok() << "\n";

  cls::TrainOptions zero_tol{};
  zero_tol.tolerance = 0.0;
  cls::SparseLinearBinaryClassifier b;
  std::cout << "tolerance=0 ok       = " << b.learn(x, y, zero_tol).ok() << "\n";

  const pluss::Series single_class = pluss::make_series(
      {1.0, 1.0, 1.0, 1.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearBinaryClassifier c;
  std::cout << "single-class y ok    = " << c.learn(x, single_class).ok() << "\n";

  cls::SparseLinearBinaryClassifier trained;
  trained.learn(x, y);
  const cls::SparseTripletMatrix wrong_cols = cls::make_sparse_triplet(1, 5, {0}, {4}, {1.0}).value();
  std::cout << "feature-count mismatch ok = " << trained.infer_scores(wrong_cols).ok() << "\n";

  return 0;
}
