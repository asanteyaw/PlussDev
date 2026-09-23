#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;

  const std::vector<std::int64_t> rows_v = {0, 1, 2};
  const std::vector<std::int64_t> cols_v = {0, 1, 2};
  const std::vector<double> vals_v = {1.0, 2.0, 3.0};

  const cls::StatusOr<cls::SparseTripletMatrix> triplet =
      cls::make_sparse_triplet(3, 3, rows_v, cols_v, vals_v);
  std::cout << "ok = " << std::boolalpha << triplet.ok() << "\n";
  std::cout << "values = " << triplet.value().values << "\n";

  return 0;
}
