#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // build() with no arguments makes an empty registry. `lymph::models::build`
  // is the main-family alias; `lymph::models::builder::build` is the same
  // function at its full scope.
  lymph::models::Builder builder = lymph::models::build();

  // inject registers one model under a name and returns the builder, so calls
  // chain. A freshly built model needs no std::move.
  builder.inject("logreg", lymph::models::logistic_regression())
         .inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}))
         .inject("knn", lymph::models::neighbor::knn_classifier({.k = 3}));

  std::cout << "size    = " << builder.size() << "\n";
  std::cout << "has knn = " << std::boolalpha << builder.has("knn") << "\n";
  const std::vector<std::string> names = builder.names();
  std::cout << "names   = " << names[0] << ", " << names[1] << ", " << names[2] << "\n";

  return 0;
}
