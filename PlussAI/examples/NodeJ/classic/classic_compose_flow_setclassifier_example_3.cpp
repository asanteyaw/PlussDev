// compose::Flow::set_classifier example 3 -- replaces any existing
// classifier/regressor outright, and clears model_spec() since there is no
// ModelSpec behind a hand-built model.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow =
      cls::make_flow(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();
  std::cout << "has_model_spec() after make_flow() = " << std::boolalpha << flow.has_model_spec() << "\n";

  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> replacement =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  flow.set_classifier(replacement.move_value());
  std::cout << "has_model_spec() after set_classifier() = " << flow.has_model_spec() << "\n";
  std::cout << "has_model() after set_classifier() = " << flow.has_model() << "\n";

  return 0;
}
