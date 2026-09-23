#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor with_default;
  with_default.learn(x, y);

  cls::LinearFamilyOptions no_intercept{};
  no_intercept.fit_intercept = false;
  cls::SparseLinearRegressor without_intercept_flag;
  without_intercept_flag.learn(x, y, {}, no_intercept);

  const double diff = (with_default.infer(x).value() - without_intercept_flag.infer(x).value())
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff (fit_intercept ignored) = " << diff << "\n";

  return 0;
}
