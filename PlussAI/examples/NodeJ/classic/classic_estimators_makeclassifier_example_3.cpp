// estimators::make_classifier example 3 -- both construction-time failure
// modes.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec wrong_task{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                  cls::LinearFamilyOptions{}};
  const cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> s1 = cls::make_classifier(wrong_task);
  std::cout << "wrong task: ok = " << std::boolalpha << s1.ok() << ", message = \""
            << s1.status().message << "\"\n";

  cls::ModelSpec wrong_options{};
  wrong_options.task = cls::TaskKind::Classification;
  wrong_options.family = cls::ModelFamily::Bayes;
  wrong_options.options = cls::TreeFamilyOptions{};
  const cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> s2 = cls::make_classifier(wrong_options);
  std::cout << "wrong options: ok = " << s2.ok() << ", message = \"" << s2.status().message << "\"\n";

  return 0;
}
