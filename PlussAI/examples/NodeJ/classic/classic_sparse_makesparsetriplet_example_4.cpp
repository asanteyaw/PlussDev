#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::StatusOr<cls::SparseTripletMatrix> triplet =
      cls::make_sparse_triplet(2, 2, {0}, {5}, {1.0});  // column 5 is outside [0, 2)
  std::cout << "ok = " << std::boolalpha << triplet.ok() << "\n";
  std::cout << "message = \"" << triplet.status().message << "\"\n";

  return 0;
}
