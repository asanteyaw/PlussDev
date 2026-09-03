#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::builder::Builder builder = lymph::models::builder::build();
  std::cout << "empty registry: size = " << builder.size()
            << ", empty = " << std::boolalpha << builder.empty()
            << ", names = " << builder.names().size() << "\n";

  builder.inject("logreg", lymph::models::logistic_regression());
  builder.inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}));

  // size() and empty() are the cheap counting siblings of names(): a count
  // and a yes/no, without building the string vector.
  std::cout << "after two injects: size = " << builder.size()
            << ", empty = " << builder.empty()
            << ", names = " << builder.names().size() << "\n";

  return 0;
}
