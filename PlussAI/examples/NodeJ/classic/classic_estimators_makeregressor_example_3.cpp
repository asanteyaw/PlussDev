// estimators::make_regressor example 3 -- both construction-time failure
// modes.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec wrong_task{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear};
  const cls::StatusOr<std::unique_ptr<cls::RegressorBase>> s1 = cls::make_regressor(wrong_task);
  std::cout << "wrong task: ok = " << std::boolalpha << s1.ok() << ", message = \""
            << s1.status().message << "\"\n";

  cls::ModelSpec wrong_options{};
  wrong_options.task = cls::TaskKind::Regression;
  wrong_options.family = cls::ModelFamily::Kernel;
  wrong_options.options = cls::EnsembleFamilyOptions{};
  const cls::StatusOr<std::unique_ptr<cls::RegressorBase>> s2 = cls::make_regressor(wrong_options);
  std::cout << "wrong options: ok = " << s2.ok() << ", message = \"" << s2.status().message << "\"\n";

  return 0;
}
