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
      {"boost", lymph::models::ensemble::gradient_boost_classifier()},
  });

  // list(n) is a quick "what's in here" preview: the first n names, in
  // insertion order.
  const std::vector<std::string> preview = builder.list(3);
  std::cout << "list(3):";
  for (const std::string& name : preview) std::cout << " " << name;
  std::cout << "\n";

  return 0;
}
