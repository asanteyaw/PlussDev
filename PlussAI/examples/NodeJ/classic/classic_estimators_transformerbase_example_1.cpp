// estimators::TransformerBase example 1 -- learn then map, versus learn_map.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 0.1, 0.2, 0.1, 10.0, 10.1, 9.9, 10.2}, {8}, f64).reshape({4, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  cls::ClusterFamilyOptions opts{};
  opts.clusters = 2;
  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> learn_then_map =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, opts});
  learn_then_map.value()->learn(batch);
  const pluss::Series mapped = learn_then_map.value()->map(batch).value();

  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> fused =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, opts});
  const pluss::Series learn_mapped = fused.value()->learn_map(batch).value();

  std::cout << "learn() then map() = " << mapped << "\n";
  std::cout << "learn_map()        = " << learn_mapped << "\n";

  return 0;
}
