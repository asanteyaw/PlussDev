#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  // row 0 gets column 0 twice: 1.0 and 1.5, which must sum to 2.5.
  const cls::SparseTripletMatrix triplet =
      cls::make_sparse_triplet(4, 3, {0, 0, 0, 2, 3}, {0, 0, 1, 0, 2}, {1.0, 1.5, 2.0, 3.0, 4.0}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(triplet).value();

  std::cout << "row_ptr = [";
  for (std::int64_t v : csr.row_ptr) std::cout << v << " ";
  std::cout << "]\n";

  for (std::int64_t r = 0; r < csr.rows; ++r) {
    std::cout << "row " << r << ": ";
    for (std::int64_t p = csr.row_ptr[static_cast<std::size_t>(r)]; p < csr.row_ptr[static_cast<std::size_t>(r + 1)]; ++p) {
      std::cout << "(col=" << csr.col_ind[static_cast<std::size_t>(p)]
                << ", val=" << csr.values[static_cast<std::size_t>(p)] << ") ";
    }
    std::cout << "\n";
  }

  return 0;
}
