#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LabeledBatch empty_batch{};
  cls::ScoreRankOptions negative_top_k{};
  negative_top_k.top_k = -1;
  const cls::StatusOr<pluss::Series> rejected = cls::f_regression_rank(empty_batch, negative_top_k);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
