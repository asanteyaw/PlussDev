#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 3;
  const cls::SplitPlan plan = cls::crossval_plan(10, options).value();
  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << ": test size = " << plan.folds[i].test_indices.numel() << "\n";
  }

  return 0;
}
