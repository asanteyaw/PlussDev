// compose::Flow::add_feature_transformer example 1 -- a pre-built
// sub-pipeline (one routed StandardScaler stage, PassThrough remainder),
// fit and applied through Flow::learn_map.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer sub(cls::prep::FeatureRemainderPolicy::PassThrough);
  sub.add_stage(std::make_unique<cls::prep::StandardScaler>(), {0});

  cls::Flow flow{};
  const cls::Status s = flow.add_feature_transformer(std::move(sub));
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_opaque_transforms() = " << flow.has_opaque_transforms() << "\n";

  const pluss::Series x = pluss::make_series({1.0, 10.0, 2.0, 20.0, 3.0, 30.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series mapped = flow.learn_map(batch).value();
  std::cout << "mapped = " << mapped << "\n";

  return 0;
}
