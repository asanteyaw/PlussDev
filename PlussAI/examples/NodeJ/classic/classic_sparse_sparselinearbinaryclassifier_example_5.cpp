#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier none;
  none.learn(x, y);

  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 1.0;
  cls::SparseLinearBinaryClassifier regularized;
  regularized.learn(x, y, {}, l2);

  std::cout << "score(row 0), no penalty = " << none.infer_scores(x).value() << "\n";
  std::cout << "score(row 0), L2 penalty = " << regularized.infer_scores(x).value() << "\n";

  return 0;
}
