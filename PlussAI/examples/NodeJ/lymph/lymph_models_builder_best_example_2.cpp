#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // Asking for the best before running a benchmark is an error: there is no
  // remembered comparison to draw a winner from.
  try {
    const lymph::models::Model winner = builder.best();
    std::cout << "unexpectedly returned a winner\n";
  } catch (const std::exception& err) {
    std::cout << "best() before benchmark() threw: " << err.what() << "\n";
  }

  return 0;
}
