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

  // The default argument (-1) means "all of them", so list() and list(-1) are
  // the same as names() here.
  const std::vector<std::string> a = builder.list();
  const std::vector<std::string> b = builder.list(-1);
  std::cout << "list()   size = " << a.size() << "\n";
  std::cout << "list(-1) size = " << b.size() << "\n";
  std::cout << "names()  size = " << builder.names().size() << "\n";

  return 0;
}
