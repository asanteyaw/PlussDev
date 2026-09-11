// families::ModelSpec example 2 -- a task/function mismatch and a
// family/options mismatch.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec regression_spec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                       cls::LinearFamilyOptions{}};
  const cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> wrong_task =
      cls::make_classifier(regression_spec);
  std::cout << "regression spec into make_classifier: ok = " << std::boolalpha << wrong_task.ok()
            << ", message = \"" << wrong_task.status().message << "\"\n";

  cls::ModelSpec mismatched{};
  mismatched.task = cls::TaskKind::Classification;
  mismatched.family = cls::ModelFamily::Tree;         // says Tree...
  mismatched.options = cls::LinearFamilyOptions{};     // ...but options holds Linear
  const cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> wrong_family =
      cls::make_classifier(mismatched);
  std::cout << "mismatched family/options: ok = " << wrong_family.ok() << ", message = \""
            << wrong_family.status().message << "\"\n";

  return 0;
}
