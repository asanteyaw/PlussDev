#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build();
  builder.inject("logreg", lymph::models::logistic_regression({.fit_intercept = true}));
  builder.inject("tree", lymph::models::decision_tree_classifier({.max_depth = 3}));

  std::cout << "== after_inject ==\n";
  std::cout << "size = " << builder.size() << ", has_tree = " << std::boolalpha
            << builder.has("tree") << "\n";
  const std::vector<std::string> names = builder.names();
  std::cout << "names = " << names[0] << ", " << names[1] << "\n\n";
  std::cout << builder.report() << "\n";

  const lymph::models::Model removed = builder.eject("tree");

  std::cout << "== after_eject ==\n";
  std::cout << "size = " << builder.size() << ", has_tree = " << builder.has("tree")
            << ", ejected_trained = " << removed.trained() << "\n";
  const std::vector<std::string> preview = builder.list(1);
  std::cout << "preview = " << preview[0] << "\n";
  std::cout << "remaining_task_kind = " << static_cast<int>(builder.get("logreg").task_kind())
            << "\n";

  return 0;
}
