#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor plain;
  plain.learn(x, y);

  cls::LinearFamilyOptions robust{};
  robust.objective = cls::LinearObjective::RobustLeast;
  robust.robust_transition = 1.0;
  cls::SparseLinearRegressor odd_objective;
  odd_objective.learn(x, y, {}, robust);

  const double diff = (plain.infer(x).value() - odd_objective.infer(x).value())
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff (objective ignored) = " << diff << "\n";

  return 0;
}
