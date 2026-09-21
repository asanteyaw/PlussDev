// compose::Flow::clear_model example 1 -- removes a Classification/
// Regression model cleanly, leaving transform stages untouched.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.set_model(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  std::cout << "before: has_model() = " << std::boolalpha << flow.has_model()
            << ", stage_count() = " << flow.stage_count() << "\n";

  flow.clear_model();
  std::cout << "after: has_model() = " << flow.has_model()
            << ", has_model_spec() = " << flow.has_model_spec()
            << ", stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
