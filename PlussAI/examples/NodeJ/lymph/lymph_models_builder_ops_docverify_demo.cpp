#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string join_names(const std::vector<std::string>& values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ", ";
    out << values[i];
  }
  return out.str();
}

} // namespace

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build();
  builder.inject("logreg", lymph::models::logistic_regression({.fit_intercept = true}));
  builder.inject("tree", lymph::models::decision_tree_classifier({.max_depth = 3}));

  std::cout << "== after_inject ==\n";
  std::cout << "size = " << builder.size()
            << ", has_tree = " << std::boolalpha << builder.has("tree") << "\n";
  std::cout << "names = " << join_names(builder.names()) << "\n\n";
  std::cout << builder.report() << "\n";

  const lymph::models::Model removed = builder.eject("tree");

  std::cout << "== after_eject ==\n";
  std::cout << "size = " << builder.size()
            << ", has_tree = " << builder.has("tree")
            << ", ejected_trained = " << removed.trained() << "\n";
  std::cout << "preview = " << join_names(builder.list(1)) << "\n";
  std::cout << "remaining_task_kind = "
            << static_cast<int>(builder.get("logreg").task_kind()) << "\n";

  return 0;
}
