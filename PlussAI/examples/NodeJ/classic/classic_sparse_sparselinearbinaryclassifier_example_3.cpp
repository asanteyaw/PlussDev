#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier default_objective;
  default_objective.learn(x, y);

  cls::LinearFamilyOptions weird{};
  weird.objective = cls::LinearObjective::LeastSquares;
  cls::SparseLinearBinaryClassifier odd_objective;
  odd_objective.learn(x, y, {}, weird);

  const double diff = (default_objective.infer_scores(x).value() - odd_objective.infer_scores(x).value())
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff (objective ignored) = " << diff << "\n";

  return 0;
}
