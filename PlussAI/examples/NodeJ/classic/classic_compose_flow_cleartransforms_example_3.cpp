// compose::Flow::clear_transforms example 3 -- safe to call on an
// already-empty flow, and does not touch an attached model.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow =
      cls::make_flow(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();

  flow.clear_transforms();  // no stages yet -- a no-op
  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  std::cout << "has_model() (untouched) = " << std::boolalpha << flow.has_model() << "\n";

  return 0;
}
