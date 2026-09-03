#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
  });

  // Ejecting a name that was never registered is an error.
  try {
    const lymph::models::Model missing = builder.eject("forest");
    std::cout << "unexpectedly ejected a model\n";
  } catch (const std::exception& err) {
    std::cout << "eject(\"forest\") threw: " << err.what() << "\n";
  }

  std::cout << "size unchanged = " << builder.size() << "\n";

  return 0;
}
