// compose::Flow::add_named_transform example 2 -- remainder = Drop removes a
// column not named by the route from the flow's output entirely.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_named_transform(cls::TransformKind::StandardScaler, cls::prep::StandardScalerOptions{},
                           "scale_col0", {0}, cls::RouteRemainderPolicy::Drop);

  const cls::TransformSpec& spec = flow.transform_specs().front();
  std::cout << "stage_key = " << spec.stage_key << "\n";
  std::cout << "remainder == Drop: " << std::boolalpha
            << (spec.remainder == cls::RouteRemainderPolicy::Drop) << "\n";

  return 0;
}
