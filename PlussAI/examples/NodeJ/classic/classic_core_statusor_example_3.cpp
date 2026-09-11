// core::StatusOr example 3 -- move_value() on a move-only T. Flow has no copy
// constructor, so StatusOr<Flow>::value() (a reference) cannot be used to
// copy-initialize a Flow; move_value() is the way to take ownership.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ModelSpec spec{};
  spec.task = cls::TaskKind::Regression;
  spec.family = cls::ModelFamily::Linear;
  spec.options = cls::LinearFamilyOptions{};

  cls::StatusOr<cls::Flow> made = cls::make_flow(spec);
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";

  cls::Flow flow = made.move_value();  // moves the Flow out; made.value() would copy-construct and fail to compile
  std::cout << "flow.has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "flow.trained()        = " << flow.trained() << "\n";

  return 0;
}
