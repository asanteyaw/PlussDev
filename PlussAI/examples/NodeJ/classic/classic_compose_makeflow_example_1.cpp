// compose::make_flow example 1 -- building a flow, move_value(), initial
// state.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made =
      cls::make_flow(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";

  cls::Flow flow = made.move_value();
  std::cout << "has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "has_model()      = " << flow.has_model() << "\n";
  std::cout << "stage_count()    = " << flow.stage_count() << "\n";
  std::cout << "trained()        = " << flow.trained() << "\n";

  return 0;
}
