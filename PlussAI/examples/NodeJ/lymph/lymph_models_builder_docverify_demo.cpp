#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression({.fit_intercept = true})},
      {"tree", lymph::models::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::knn_classifier({.k = 3})},
  });

  std::cout << "== builder ==\n";
  std::cout << "size = " << builder.size() << ", empty = " << std::boolalpha << builder.empty()
            << ", has_tree = " << builder.has("tree") << "\n";
  const std::vector<std::string> names = builder.names();
  const std::vector<std::string> preview = builder.list(2);
  std::cout << "names = " << names[0] << ", " << names[1] << ", " << names[2] << "\n";
  std::cout << "preview = " << preview[0] << ", " << preview[1] << "\n\n";
  std::cout << builder.report() << "\n";

  return 0;
}
