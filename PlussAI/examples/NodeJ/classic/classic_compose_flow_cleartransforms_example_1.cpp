// compose::Flow::clear_transforms example 1 -- removes every stage,
// including opaque ones, and resets has_opaque_transforms().

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.append_transform(std::make_unique<cls::prep::OneHotEncoder>());
  std::cout << "before: stage_count() = " << flow.stage_count()
            << ", has_opaque_transforms() = " << std::boolalpha << flow.has_opaque_transforms() << "\n";

  flow.clear_transforms();
  std::cout << "after: stage_count() = " << flow.stage_count()
            << ", has_opaque_transforms() = " << flow.has_opaque_transforms() << "\n";

  return 0;
}
