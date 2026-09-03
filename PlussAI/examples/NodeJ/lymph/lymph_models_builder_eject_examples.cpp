#include <pluss/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  {
    auto builder = lymph::models::build({
        {"logreg", lymph::models::linear::logistic_regression()},
        {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
    });
    const lymph::models::Model removed = builder.eject("tree");
    std::cout << "example_1: has_tree = " << std::boolalpha << builder.has("tree")
              << ", removed_trained = " << removed.trained() << "\n";
  }

  {
    auto builder = lymph::models::build({
        {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
    });
    [[maybe_unused]] const lymph::models::Model old_tree = builder.eject("tree");
    builder.inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 4}));
    std::cout << "example_2: replacement_present = " << std::boolalpha << builder.has("tree")
              << ", size = " << builder.size() << "\n";
  }

  {
    auto builder = lymph::models::build({
        {"logreg", lymph::models::linear::logistic_regression()},
        {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
        {"knn", lymph::models::knn_classifier({.k = 3})},
    });
    [[maybe_unused]] const lymph::models::Model removed = builder.eject("tree");
    const auto names = builder.names();
    std::cout << "example_3: first = " << names[0] << ", second = " << names[1]
              << ", size = " << builder.size() << "\n";
  }

  {
    auto builder = lymph::models::build();
    bool missing_rejected = false;
    try {
      [[maybe_unused]] const lymph::models::Model removed = builder.eject("missing");
    } catch (const std::exception &) {
      missing_rejected = true;
    }
    std::cout << "example_4: missing_rejected = " << std::boolalpha << missing_rejected
              << ", empty = " << builder.empty() << "\n";
  }

  return 0;
}
