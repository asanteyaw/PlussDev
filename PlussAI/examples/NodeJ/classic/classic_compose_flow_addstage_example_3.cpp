// compose::Flow::add_stage example 3 -- chaining several typed stages into a
// mixed-type pipeline.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0, 1});   // two numeric columns
  flow.add_one_hot_encoder({}, {2});      // one categorical column

  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  for (const cls::TransformSpec& spec : flow.transform_specs()) {
    std::cout << "kind = " << static_cast<int>(spec.kind)
              << ", route size = " << spec.feature_route.size() << "\n";
  }

  return 0;
}
