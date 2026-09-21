// compose::Flow::append_transform example 3 -- mixing a named stage, an
// opaque append_transform stage, and another named stage: transform_specs()
// carries an empty placeholder at the opaque stage's index to stay aligned
// with the underlying transformer list.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});                                    // named, index 0
  flow.append_transform(std::make_unique<cls::prep::OneHotEncoder>());  // opaque, index 1
  flow.add_one_hot_encoder({}, {2});                                    // named, index 2

  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  const std::vector<cls::TransformSpec>& specs = flow.transform_specs();
  for (std::size_t i = 0; i < specs.size(); ++i) {
    std::cout << "spec[" << i << "]: route size = " << specs[i].feature_route.size() << "\n";
  }
  std::cout << "has_opaque_transforms() = " << std::boolalpha << flow.has_opaque_transforms() << "\n";

  return 0;
}
