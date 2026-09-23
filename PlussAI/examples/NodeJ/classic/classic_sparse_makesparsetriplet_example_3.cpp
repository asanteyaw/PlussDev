#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  // (0, 0) is listed twice: 1.0 and 1.5 -- both survive as separate
  // stored entries in the CSR result.
  const cls::SparseTripletMatrix triplet =
      cls::make_sparse_triplet(2, 2, {0, 0, 1}, {0, 0, 1}, {1.0, 1.5, 2.0}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(triplet).value();

  std::cout << "nnz stored (still 3, nothing merged) = " << csr.values.size() << "\n";
  std::cout << "(0,0) first stored value  = " << csr.values[0] << "\n";
  std::cout << "(0,0) second stored value = " << csr.values[1] << "\n";

  return 0;
}
