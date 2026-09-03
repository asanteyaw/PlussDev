#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  // The same bulk form, this time from a std::unordered_map. Useful when the
  // registry is assembled by name lookup and insertion order never mattered.
  // inject accepts it directly; because an unordered_map has no meaningful
  // iteration order, the resulting registration order is unspecified -- sort
  // names() if you need a stable view.
  std::unordered_map<std::string, lymph::models::Model> pool;
  pool.emplace("logreg", lymph::models::logistic_regression());
  pool.emplace("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2}));
  pool.emplace("gbm", lymph::models::ensemble::gradient_boost_classifier());

  // Full-scope spelling on the alias-free path.
  lymph::models::builder::Builder builder = lymph::models::builder::build();
  builder.inject(pool);

  std::vector<std::string> names = builder.names();
  std::sort(names.begin(), names.end());

  std::cout << "size = " << builder.size() << "\n";
  for (const std::string& name : names) std::cout << "  - " << name << "\n";
  std::cout << "has gbm = " << std::boolalpha << builder.has("gbm") << "\n";

  return 0;
}
