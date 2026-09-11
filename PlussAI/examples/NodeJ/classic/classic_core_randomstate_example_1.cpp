// core::RandomState example 1 -- the same seed reproduces the same fold
// assignment; a different seed does not.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.shuffle = true;
  split.random.seed = 7;
  split.random.deterministic = true;

  const cls::StatusOr<cls::SplitPlan> a = cls::crossval_plan(9, split);
  const cls::StatusOr<cls::SplitPlan> b = cls::crossval_plan(9, split);  // same seed
  split.random.seed = 8;
  const cls::StatusOr<cls::SplitPlan> c = cls::crossval_plan(9, split);  // different seed

  auto first_test_row = [](const cls::StatusOr<cls::SplitPlan>& p) {
    return p.ok() ? p.value().folds.front().test_indices.slice(0, 0, 1).item<double>() : -1.0;
  };

  std::cout << "a first test row = " << first_test_row(a) << "\n";
  std::cout << "b first test row = " << first_test_row(b) << "  (same seed as a)\n";
  std::cout << "c first test row = " << first_test_row(c) << "  (different seed)\n";

  return 0;
}
