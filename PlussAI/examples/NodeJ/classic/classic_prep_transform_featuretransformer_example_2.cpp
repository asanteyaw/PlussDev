#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer transformer;  // remainder = Drop by default
  std::vector<std::unique_ptr<cls::TransformerBase>> pipeline{};
  pipeline.push_back(std::make_unique<cls::prep::PowerMap>());
  pipeline.push_back(std::make_unique<cls::prep::StandardScaler>());
  transformer.add_pipeline(std::move(pipeline), {0});

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0, 4.0, 8.0}, {4}, f64).reshape({4, 1});
  const pluss::Series mapped = transformer.learn_map(batch).value();
  std::cout << "map = " << mapped << "\n";

  return 0;
}
