// compose::Flow::set_model example 1 -- attaching a Classification model to
// an already-built transform pipeline (the pattern make_flow shortcuts).

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  const cls::Status s =
      flow.set_model(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
