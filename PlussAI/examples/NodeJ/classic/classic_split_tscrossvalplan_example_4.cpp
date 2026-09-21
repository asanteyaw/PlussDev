#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions options{};
  options.shuffle = true;
  const cls::StatusOr<cls::SplitPlan> plan = cls::ts_crossval_plan(10, options);
  std::cout << "ok = " << std::boolalpha << plan.ok() << "\n";
  std::cout << "message = \"" << plan.status().message << "\"\n";

  return 0;
}
