#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // best() and worst() throw if no comparison has been run. Guard them with
  // benchmarked() when a caller might reach for a winner before benchmarking.
  if (builder.benchmarked()) {
    std::cout << "winner is ready\n";
  } else {
    std::cout << "no benchmark yet -- call builder.benchmark(...) first\n";
  }

  return 0;
}
