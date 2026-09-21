#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 3;
  const cls::SplitPlan plan = cls::crossval_plan(10, options).value();

  std::cout << "fold count = " << plan.folds.size() << "\n";
  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    const cls::IndexSplit& fold = plan.folds[i];
    std::cout << "fold " << i << ": train=" << fold.train_indices.numel()
              << " test=" << fold.test_indices.numel() << "\n";
  }

  return 0;
}
