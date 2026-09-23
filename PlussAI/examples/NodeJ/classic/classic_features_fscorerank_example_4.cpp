#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LabeledBatch empty_batch{};
  cls::ScoreRankOptions zero_top_k{};
  zero_top_k.top_k = 0;
  const cls::StatusOr<pluss::Series> rejected = cls::f_score_rank(empty_batch, zero_top_k);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
