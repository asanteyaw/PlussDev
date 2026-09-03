#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // The batch form takes a braced list of {name, model} pairs and validates
  // the whole set before touching the registry, so a bad entry cannot leave a
  // half-populated Builder behind. `build({...})` is a construction-time
  // shortcut for exactly this.
  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  // inject can also take a braced list to add several at once after construction.
  builder.inject({
      {"forest", lymph::models::ensemble::random_forest_classifier()},
      {"boost", lymph::models::ensemble::gradient_boost_classifier()},
  });

  std::cout << "size = " << builder.size() << "\n";
  for (const std::string& name : builder.names()) std::cout << "  - " << name << "\n";

  return 0;
}
