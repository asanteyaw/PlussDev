#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      3, 1, {0, 1, 2}, {0, 0, 0}, {1.0, 2.0, 3.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 1.0, 5.0}, {3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier model;
  const cls::Status learned = model.learn(x, y);
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  // row 1 (y=1.0) was folded into the same class as row 0 (y=0.0) at fit
  // time -- both should score on the same side.
  std::cout << "infer = " << model.infer(x).value() << "\n";

  return 0;
}
