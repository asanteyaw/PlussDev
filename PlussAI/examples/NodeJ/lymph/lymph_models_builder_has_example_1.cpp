#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
  });

  // has is a cheap name lookup: no copy, no throw when the name is absent.
  std::cout << "has logreg  = " << std::boolalpha << builder.has("logreg") << "\n";
  std::cout << "has tree    = " << builder.has("tree") << "\n";
  std::cout << "has forest  = " << builder.has("forest") << "\n";

  (void)builder.eject("tree");
  std::cout << "has tree after eject = " << builder.has("tree") << "\n";

  return 0;
}
