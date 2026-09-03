#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
  });

  // Asking for more names than exist is not an error -- list clamps to the
  // registry size.
  const std::vector<std::string> over = builder.list(10);
  std::cout << "list(10) on a size-2 registry returned " << over.size() << " names:";
  for (const std::string& name : over) std::cout << " " << name;
  std::cout << "\n";

  return 0;
}
