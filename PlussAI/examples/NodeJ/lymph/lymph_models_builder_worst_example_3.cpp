#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::builder::Builder builder = lymph::models::builder::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // Like best(), worst() needs a prior benchmark and throws without one.
  try {
    const lymph::models::Model laggard = builder.worst();
    std::cout << "unexpectedly returned a model\n";
  } catch (const std::exception& err) {
    std::cout << "worst() before benchmark() threw: " << err.what() << "\n";
  }

  return 0;
}
