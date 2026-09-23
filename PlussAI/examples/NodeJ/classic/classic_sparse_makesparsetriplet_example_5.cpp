#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const cls::StatusOr<cls::SparseTripletMatrix> triplet =
      cls::make_sparse_triplet(2, 2, {0}, {0}, {nan});
  std::cout << "ok = " << std::boolalpha << triplet.ok() << "\n";
  std::cout << "message = \"" << triplet.status().message << "\"\n";

  return 0;
}
