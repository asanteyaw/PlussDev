#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  // get throws on an unknown name, so guard it with has when the name comes
  // from outside the code.
  const std::vector<std::string> wanted = {"logreg", "tree", "knn"};
  for (const std::string& name : wanted) {
    if (builder.has(name)) {
      std::cout << name << ": present, task_kind = "
                << static_cast<int>(builder.get(name).task_kind()) << "\n";
    } else {
      std::cout << name << ": not registered\n";
    }
  }

  return 0;
}
