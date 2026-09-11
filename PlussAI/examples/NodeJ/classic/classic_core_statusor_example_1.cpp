// core::StatusOr example 1 -- StatusOr<T> as it actually arrives: the return
// value of a real classic call, checked with ok() before its value is used.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 10;  // more folds than samples below -- must fail

  const cls::StatusOr<cls::SplitPlan> bad = cls::crossval_plan(4, split);
  std::cout << "bad.ok()      = " << std::boolalpha << bad.ok() << "\n";
  std::cout << "bad.status()  = code " << static_cast<int>(bad.status().code) << ", \""
            << bad.status().message << "\"\n";

  split.folds = 4;
  const cls::StatusOr<cls::SplitPlan> good = cls::crossval_plan(4, split);
  std::cout << "good.ok()     = " << good.ok() << "\n";
  if (good.ok()) {
    std::cout << "good fold count = " << good.value().folds.size() << "\n";
  }

  return 0;
}
