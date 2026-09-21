#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear,
                     cls::LinearFamilyOptions{.objective = cls::LinearObjective::Logistic}});

  cls::Flow flow;
  flow.set_classifier(made.move_value());
  const cls::StatusOr<double> result = flow.nested_param("model::task");
  std::cout << "ok = " << std::boolalpha << result.ok() << "\n";
  std::cout << "message = \"" << result.status().message << "\"\n";

  return 0;
}
