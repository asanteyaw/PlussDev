#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build();
  builder.inject("logreg", lymph::models::logistic_regression());
  builder.inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}));
  builder.inject("knn", lymph::models::neighbor::knn_classifier({.k = 3}));
  builder.inject("forest", lymph::models::ensemble::random_forest_classifier());

  // names returns every registered name in the order it was injected -- the
  // full identity of the registry, not a preview.
  std::cout << "count = " << builder.names().size() << "\n";
  for (const std::string& name : builder.names()) std::cout << "  " << name << "\n";

  return 0;
}
