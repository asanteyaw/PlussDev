// families::ModelSpec example 3 -- a candidate list, the shape sweep/tune
// consume, built without constructing any estimator.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions light{};
  light.regularization_strength = 1e-6;
  cls::LinearFamilyOptions heavy{};
  heavy.regularization_strength = 1e2;

  const std::vector<cls::ModelSpec> candidates = {
      {cls::TaskKind::Regression, cls::ModelFamily::Linear, light},
      {cls::TaskKind::Regression, cls::ModelFamily::Linear, heavy},
      {cls::TaskKind::Regression, cls::ModelFamily::Tree, cls::TreeFamilyOptions{}},
  };

  std::cout << "candidate count = " << candidates.size() << "\n";
  for (const cls::ModelSpec& spec : candidates) {
    std::cout << "  family = " << static_cast<int>(spec.family)
              << ", task = " << static_cast<int>(spec.task) << "\n";
  }

  return 0;
}
