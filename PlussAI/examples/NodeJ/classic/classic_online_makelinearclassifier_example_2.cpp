#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Hinge;
  options.family.regularization = cls::RegularizationKind::L1;

  const cls::StatusOr<cls::online::OnlineLinearClassifier> made =
      cls::online::make_online_linear_classifier(options);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
