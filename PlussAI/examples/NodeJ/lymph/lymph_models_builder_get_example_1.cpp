#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
  });

  // get returns a reference to the stored model without removing it. The
  // registry still holds every entry afterward.
  const lymph::models::Model& tree = builder.get("tree");
  std::cout << "tree task_kind   = " << static_cast<int>(tree.task_kind()) << "\n";
  std::cout << "tree trained     = " << std::boolalpha << tree.trained() << "\n";
  std::cout << "tree stage_count = " << tree.stage_count() << "\n\n";

  std::cout << "registry still holds: size = " << builder.size()
            << ", has tree = " << builder.has("tree") << "\n";

  return 0;
}
