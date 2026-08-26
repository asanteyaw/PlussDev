#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression({.fit_intercept = true})},
      {"tree", lymph::models::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::knn_classifier({.k = 3})},
  });

  std::cout << "== builder ==\n";
  std::cout << "size=" << builder.size()
            << " empty=" << std::boolalpha << builder.empty() << "\n";
  std::cout << builder.report() << "\n";

  return 0;
}
