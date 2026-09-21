#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions same_seed{};
  same_seed.random.seed = 42;
  const cls::IndexSplit first = cls::holdout_split(10, same_seed).value();
  const cls::IndexSplit second = cls::holdout_split(10, same_seed).value();
  std::cout << "first  = " << first.test_indices << "\n";
  std::cout << "second = " << second.test_indices << "\n";

  cls::SplitOptions other_seed{};
  other_seed.random.seed = 7;
  const cls::IndexSplit third = cls::holdout_split(10, other_seed).value();
  std::cout << "third  = " << third.test_indices << "\n";

  return 0;
}
