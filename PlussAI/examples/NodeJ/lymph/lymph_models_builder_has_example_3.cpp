#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // A fresh registry holds nothing, so has is always false and empty is true.
  lymph::models::builder::Builder builder = lymph::models::builder::build();
  std::cout << "empty        = " << std::boolalpha << builder.empty() << "\n";
  std::cout << "has anything = " << builder.has("logreg") << "\n";

  builder.inject("logreg", lymph::models::logistic_regression());
  std::cout << "after inject: empty = " << builder.empty()
            << ", has logreg = " << builder.has("logreg") << "\n";

  return 0;
}
