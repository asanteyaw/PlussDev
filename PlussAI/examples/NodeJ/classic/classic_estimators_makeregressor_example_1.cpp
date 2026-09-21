// estimators::make_regressor example 1 -- one spec per regression-capable
// family.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec linear_spec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                   cls::LinearFamilyOptions{}};
  const cls::ModelSpec tree_spec{cls::TaskKind::Regression, cls::ModelFamily::Tree, cls::TreeFamilyOptions{}};
  const cls::ModelSpec ensemble_spec{cls::TaskKind::Regression, cls::ModelFamily::Ensemble,
                                     cls::EnsembleFamilyOptions{}};

  std::cout << "linear ok   = " << std::boolalpha << cls::make_regressor(linear_spec).ok() << "\n";
  std::cout << "tree ok     = " << cls::make_regressor(tree_spec).ok() << "\n";
  std::cout << "ensemble ok = " << cls::make_regressor(ensemble_spec).ok() << "\n";

  return 0;
}
