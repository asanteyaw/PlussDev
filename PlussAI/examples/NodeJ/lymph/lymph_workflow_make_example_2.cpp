#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  // make(model, policy) -- the everyday shape: a chosen Model, an optional
  // Policy (defaults to Policy{}, admit-everything).
  workflow::Policy policy{};
  policy.drop({"customer_id"});

  const workflow::Workflow wf = workflow::make(lymph::models::logistic_regression(), policy);
  std::cout << "has_model = " << std::boolalpha << wf.has_model()
            << "  trained = " << wf.trained() << "\n";

  const workflow::Workflow bare = workflow::make(lymph::models::logistic_regression());
  std::cout << "bare.policy().empty() = " << bare.policy().empty() << "\n";

  return 0;
}
