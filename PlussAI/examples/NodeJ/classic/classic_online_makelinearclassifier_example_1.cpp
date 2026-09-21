#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::StatusOr<cls::online::OnlineLinearClassifier> default_options =
      cls::online::make_online_linear_classifier();
  std::cout << "default OnlineLinearOptions ok = " << std::boolalpha << default_options.ok() << "\n";

  cls::online::OnlineLinearOptions logistic{};
  logistic.family.objective = cls::LinearObjective::Logistic;
  const cls::StatusOr<cls::online::OnlineLinearClassifier> made =
      cls::online::make_online_linear_classifier(logistic);
  std::cout << "Logistic objective ok           = " << made.ok() << "\n";
  std::cout << "trained() on a fresh classifier  = " << made.value().trained() << "\n";

  return 0;
}
