// compose::Flow::set_regressor example 3 -- replaces an existing classifier
// outright: set_classifier and set_regressor are mutually exclusive, not
// additive.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions classifier_opts{};
  classifier_opts.objective = cls::LinearObjective::Logistic;
  cls::Flow flow{};
  flow.set_classifier(
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, classifier_opts})
          .move_value());
  std::cout << "task_kind() after set_classifier() == Classification: "
            << std::boolalpha << (flow.task_kind() == cls::TaskKind::Classification) << "\n";

  cls::LinearFamilyOptions regressor_opts{};
  flow.set_regressor(
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, regressor_opts})
          .move_value());
  std::cout << "task_kind() after set_regressor() == Regression: "
            << (flow.task_kind() == cls::TaskKind::Regression) << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";

  return 0;
}
