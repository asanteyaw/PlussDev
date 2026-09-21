// compose::Flow::add_feature_transformer example 2 -- two routed stages plus
// the default Drop remainder: the third column, named by no stage, is
// dropped from the output entirely.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer sub{};  // default remainder = Drop
  sub.add_stage(std::make_unique<cls::prep::StandardScaler>(), {0});
  sub.add_stage(std::make_unique<cls::prep::StandardScaler>(), {1});
  std::cout << "sub.stage_count() = " << sub.stage_count() << "\n";
  std::cout << "sub.remainder_policy() == Drop: "
            << std::boolalpha << (sub.remainder_policy() == cls::prep::FeatureRemainderPolicy::Drop) << "\n";

  cls::Flow flow{};
  flow.add_feature_transformer(std::move(sub));

  // 3 columns in; column 2 is named by no stage.
  const pluss::Series x =
      pluss::make_series({1.0, 10.0, 100.0, 2.0, 20.0, 200.0, 3.0, 30.0, 300.0}, {9}, f64).reshape({3, 3});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series mapped = flow.learn_map(batch).value();
  std::cout << "mapped = " << mapped << "\n";

  return 0;
}
