#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor none;
  none.learn(x, y);

  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 1.0;
  cls::SparseLinearRegressor regularized;
  regularized.learn(x, y, {}, l2);

  std::cout << "infer, no penalty = " << none.infer(x).value() << "\n";
  std::cout << "infer, L2 penalty = " << regularized.infer(x).value() << "\n";

  return 0;
}
