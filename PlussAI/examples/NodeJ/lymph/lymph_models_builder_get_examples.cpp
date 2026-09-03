#include <pluss/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  {
    const auto builder = lymph::models::build({
        {"logreg", lymph::models::linear::logistic_regression()},
    });
    const lymph::models::Model& model = builder.get("logreg");
    std::cout << "example_1: const_trained = " << std::boolalpha << model.trained()
              << ", task_kind = " << static_cast<int>(model.task_kind()) << "\n";
  }

  {
    auto builder = lymph::models::build({
        {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
    });
    builder.get("tree").standard_scaler();
    std::cout << "example_2: mutable_lookup = " << std::boolalpha << builder.has("tree")
              << ", trained = " << builder.get("tree").trained() << "\n";
  }

  {
    auto builder = lymph::models::build({
        {"logreg", lymph::models::linear::logistic_regression()},
        {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
    });
    const lymph::models::Model& selected = builder.get("tree");
    std::cout << "example_3: selected_is_trained = " << std::boolalpha << selected.trained()
              << ", registry_size = " << builder.size() << "\n";
  }

  {
    const auto builder = lymph::models::build();
    bool missing_rejected = false;
    try {
      [[maybe_unused]] const lymph::models::Model& model = builder.get("missing");
    } catch (const std::exception&) {
      missing_rejected = true;
    }
    std::cout << "example_4: missing_rejected = " << std::boolalpha << missing_rejected
              << ", empty = " << builder.empty() << "\n";
  }

  return 0;
}
