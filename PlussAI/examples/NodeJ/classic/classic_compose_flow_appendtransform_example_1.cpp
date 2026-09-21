// compose::Flow::append_transform example 1 -- attaching any TransformerBase
// directly, bypassing TransformKind entirely, then fitting/applying it.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  const cls::Status s = flow.append_transform(std::make_unique<cls::prep::StandardScaler>());
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  std::cout << "has_opaque_transforms() = " << flow.has_opaque_transforms() << "\n";

  const pluss::Series x = pluss::make_series({1.0, 10.0, 2.0, 20.0, 3.0, 30.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series mapped = flow.learn_map(batch).value();
  std::cout << "mapped = " << mapped << "\n";

  return 0;
}
