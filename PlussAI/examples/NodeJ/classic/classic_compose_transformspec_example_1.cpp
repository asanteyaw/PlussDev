// compose::TransformSpec example 1 -- two routed stages, read back.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});               // column 0 only
  flow.add_one_hot_encoder({}, {1});                // column 1 only

  const std::vector<cls::TransformSpec>& specs = flow.transform_specs();
  std::cout << "stage count = " << specs.size() << "\n";
  for (const cls::TransformSpec& spec : specs) {
    std::cout << "kind = " << static_cast<int>(spec.kind)
              << ", route size = " << spec.feature_route.size()
              << ", remainder = " << static_cast<int>(spec.remainder) << "\n";
  }

  return 0;
}
