#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.folds = 3;
  options.time_max_train_size = 4;
  options.shuffle = false;  // required by ts_crossval_plan
  const cls::SplitPlan plan = cls::ts_crossval_plan(12, options).value();

  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << ": train size=" << plan.folds[i].train_indices.numel()
              << " test size=" << plan.folds[i].test_indices.numel() << "\n";
  }

  return 0;
}
