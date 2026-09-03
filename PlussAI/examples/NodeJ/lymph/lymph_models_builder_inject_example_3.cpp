#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // Full-scope spelling: the implementation lives in lymph::models::builder,
  // and this works identically to the lymph::models::build() alias.
  lymph::models::builder::Builder builder = lymph::models::builder::build();
  builder.inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}));

  // Re-injecting a name that already exists is a mistake, not a silent
  // overwrite -- inject throws so a typo colliding with an existing entry is
  // caught at once.
  try {
    builder.inject("tree", lymph::models::logistic_regression());
  } catch (const std::exception& err) {
    std::cout << "second inject(\"tree\") threw: " << err.what() << "\n";
  }

  // The deliberate way to replace an entry: eject the old one, then inject.
  const lymph::models::Model removed = builder.eject("tree");
  builder.inject("tree", lymph::models::logistic_regression());
  std::cout << "after replace: size = " << builder.size()
            << ", has tree = " << std::boolalpha << builder.has("tree") << "\n";

  return 0;
}
