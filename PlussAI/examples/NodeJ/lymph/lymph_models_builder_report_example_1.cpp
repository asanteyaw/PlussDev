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

  // report returns a DataFrame -- one row per held model, with its task kind,
  // whether it is trained, and how many preparation stages it carries. Stream
  // it like any other frame.
  std::cout << builder.report();

  return 0;
}
