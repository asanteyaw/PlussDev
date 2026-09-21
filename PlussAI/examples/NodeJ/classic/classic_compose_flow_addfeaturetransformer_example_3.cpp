// compose::Flow::add_feature_transformer example 3 -- map() before learn()
// is rejected: an unfitted FeatureTransformer's own precondition, surfaced
// through Flow::map.

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
  flow.add_feature_transformer(std::move(sub));

  const pluss::Series x = pluss::make_series({1.0, 10.0, 2.0, 20.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  // map() without a prior learn()/learn_map() call.
  const cls::StatusOr<pluss::Series> mapped = flow.map(batch);
  std::cout << "map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "map message = \"" << mapped.status().message << "\"\n";

  return 0;
}
