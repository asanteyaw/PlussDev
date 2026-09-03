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

  // names is the complete list; list(n) is a bounded preview of the first n.
  const std::vector<std::string> all = builder.names();
  const std::vector<std::string> head = builder.list(2);

  std::cout << "names (" << all.size() << "): ";
  for (std::size_t i = 0; i < all.size(); ++i) std::cout << (i ? ", " : "") << all[i];
  std::cout << "\n";

  std::cout << "list(2) (" << head.size() << "): ";
  for (std::size_t i = 0; i < head.size(); ++i) std::cout << (i ? ", " : "") << head[i];
  std::cout << "\n";

  return 0;
}
