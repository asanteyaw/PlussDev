#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix triplet =
      cls::make_sparse_triplet(4, 3, {0, 0, 2, 3}, {0, 1, 0, 2}, {1.0, 2.0, 3.0, 4.0}).value();

  std::cout << "rows = " << triplet.rows << ", cols = " << triplet.cols << "\n";
  std::cout << "row_indices    = " << triplet.row_indices << "\n";
  std::cout << "column_indices = " << triplet.column_indices << "\n";
  std::cout << "values         = " << triplet.values << "\n";

  return 0;
}
