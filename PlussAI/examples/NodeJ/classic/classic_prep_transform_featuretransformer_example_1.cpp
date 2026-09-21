#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer transformer(cls::prep::FeatureRemainderPolicy::PassThrough);
  transformer.add_stage(std::make_unique<cls::prep::StandardScaler>(), {0});
  transformer.add_stage(std::make_unique<cls::prep::OneHotEncoder>(), {1});

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series(
      {0.0, 1.0, 100.0, 10.0, 2.0, 200.0}, {6}, f64).reshape({2, 3});
  std::cout << "stage_count() = " << transformer.stage_count() << "\n";
  std::cout << "map = " << transformer.learn_map(batch).value() << "\n";

  return 0;
}
