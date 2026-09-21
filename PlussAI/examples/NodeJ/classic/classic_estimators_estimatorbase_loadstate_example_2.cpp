// estimators::EstimatorBase::load_state example 2 -- the wrong tensor count.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  const std::vector<pluss::Series> empty_state{};
  const cls::Status loaded = made.value()->load_state(empty_state);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.message << "\"\n";
  std::cout << "trained() after the failed load = " << made.value()->trained() << "\n";

  return 0;
}
