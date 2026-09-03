#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
      {"forest", lymph::models::ensemble::random_forest_classifier()},
  });

  // report(n) is the tabular counterpart of list(n): only the first n rows.
  std::cout << builder.report(2);

  return 0;
}
