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
            << " empty=" << std::boolalpha << builder.empty()
            << " has_tree=" << builder.has("tree") << "\n";
  const std::vector<std::string> names = builder.names();
  const std::vector<std::string> preview = builder.list(2);
  std::cout << "names=";
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i > 0) std::cout << ",";
    std::cout << names[i];
  }
  std::cout << "\n";
  std::cout << "preview=";
  for (std::size_t i = 0; i < preview.size(); ++i) {
    if (i > 0) std::cout << ",";
    std::cout << preview[i];
  }
  std::cout << "\n";
  std::cout << builder.report() << "\n";

  return 0;
}
