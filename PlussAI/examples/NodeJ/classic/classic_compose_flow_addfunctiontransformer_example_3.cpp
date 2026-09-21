// compose::Flow::add_function_transformer example 3 -- the practical
// consequence of has_opaque_transforms(): clone() refuses an opaque-stage
// flow.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::prep::FunctionTransformer identity(
      [](const pluss::Series& x) -> cls::StatusOr<pluss::Series> { return cls::StatusOr<pluss::Series>(x); });

  cls::Flow flow{};
  flow.add_function_transformer(std::move(identity));
  std::cout << "has_opaque_transforms() = " << std::boolalpha << flow.has_opaque_transforms() << "\n";

  const cls::StatusOr<cls::Flow> cloned = flow.clone();
  std::cout << "clone ok = " << cloned.ok() << "\n";
  std::cout << "clone message = \"" << cloned.status().message << "\"\n";

  return 0;
}
