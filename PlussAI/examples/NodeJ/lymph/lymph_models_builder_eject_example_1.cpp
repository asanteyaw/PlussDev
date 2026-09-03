#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  // eject removes the entry and hands the Model back to you. The Builder no
  // longer holds it.
  const lymph::models::Model taken = builder.eject("tree");

  std::cout << "size after eject   = " << builder.size() << "\n";
  std::cout << "has tree           = " << std::boolalpha << builder.has("tree") << "\n";
  std::cout << "ejected task_kind  = " << static_cast<int>(taken.task_kind()) << "\n";
  std::cout << "ejected trained    = " << taken.trained() << "\n";

  return 0;
}
