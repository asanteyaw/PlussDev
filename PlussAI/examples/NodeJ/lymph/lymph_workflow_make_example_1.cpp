#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  // make(policy) alone starts from an empty, model-free template -- a
  // route/Policy design that has not yet picked a model. See
  // Workflow::with for attaching one later.
  workflow::Policy policy{};
  policy.drop({"customer_id"});

  const workflow::Workflow wf = workflow::make(policy);
  std::cout << "has_model = " << std::boolalpha << wf.has_model()
            << "  policy.drop_columns[0] = " << wf.policy().drop_columns.front() << "\n";

  return 0;
}
