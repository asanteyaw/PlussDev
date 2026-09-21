#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 2;
  options.shuffle = false;
  const cls::SplitPlan plan = cls::group_crossval_plan({0, 0, 1, 1, 2, 2, 3, 3}, options).value();

  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << " test_indices = " << plan.folds[i].test_indices << "\n";
  }

  return 0;
}
