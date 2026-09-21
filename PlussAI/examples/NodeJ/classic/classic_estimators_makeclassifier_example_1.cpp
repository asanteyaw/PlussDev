// estimators::make_classifier example 1 -- one spec per classification-
// capable family.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;

  const cls::ModelSpec linear_spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear};
  const cls::ModelSpec tree_spec{cls::TaskKind::Classification, cls::ModelFamily::Tree, cls::TreeFamilyOptions{}};
  const cls::ModelSpec bayes_spec{cls::TaskKind::Classification, cls::ModelFamily::Bayes, cls::BayesFamilyOptions{}};

  std::cout << "linear ok = " << std::boolalpha << cls::make_classifier(linear_spec).ok() << "\n";
  std::cout << "tree ok   = " << cls::make_classifier(tree_spec).ok() << "\n";
  std::cout << "bayes ok  = " << cls::make_classifier(bayes_spec).ok() << "\n";

  return 0;
}
