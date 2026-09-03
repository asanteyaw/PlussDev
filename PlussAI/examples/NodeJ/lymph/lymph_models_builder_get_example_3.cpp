#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
  });

  // get on an unknown name throws rather than returning a default.
  try {
    const lymph::models::Model& missing = builder.get("tree");
    std::cout << "unexpectedly got a model\n";
  } catch (const std::exception& err) {
    std::cout << "get(\"tree\") threw: " << err.what() << "\n";
  }

  return 0;
}
