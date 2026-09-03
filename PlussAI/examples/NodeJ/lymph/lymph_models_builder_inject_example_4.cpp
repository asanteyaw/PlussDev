#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <map>
#include <string>

int main() {
  namespace lymph = pluss::ai::lymph;

  // A preassembled registry: a std::map built up elsewhere (here, a short
  // loop standing in for a config-driven assembly) and handed to inject in
  // one call. Map keys are already unique, so inject skips the intra-batch
  // duplicate scan the braced-list form runs -- it still validates every
  // entry (empty name, name already registered) before adding any.
  std::map<std::string, lymph::models::Model> planned;
  planned.emplace("logreg", lymph::models::logistic_regression());
  planned.emplace("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}));
  planned.emplace("knn", lymph::models::neighbor::knn_classifier({.k = 3}));

  lymph::models::Builder builder = lymph::models::build();
  builder.inject(planned);

  // std::map iterates in sorted key order, so that is the registration order.
  std::cout << "size = " << builder.size() << "\n";
  for (const std::string& name : builder.names()) std::cout << "  - " << name << "\n";

  // Validation covers the whole map before any entry is added: a map that
  // collides with an existing name registers nothing.
  std::map<std::string, lymph::models::Model> conflicting;
  conflicting.emplace("forest", lymph::models::ensemble::random_forest_classifier());
  conflicting.emplace("tree", lymph::models::logistic_regression());
  try {
    builder.inject(conflicting);
  } catch (const std::exception& err) {
    std::cout << "conflicting map threw: " << err.what() << "\n";
  }
  std::cout << "size after failed inject = " << builder.size()
            << ", has forest = " << std::boolalpha << builder.has("forest") << "\n";

  return 0;
}
