#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(x).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor via_triplet;
  via_triplet.learn(x, y);
  cls::SparseLinearRegressor via_csr;
  via_csr.learn(csr, y);

  const double diff = (via_triplet.infer(x).value() - via_csr.infer(csr).value())
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff between the two forms = " << diff << "\n";

  return 0;
}
