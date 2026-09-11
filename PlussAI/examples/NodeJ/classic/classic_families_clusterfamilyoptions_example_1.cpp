// families::ClusterFamilyOptions example 1 -- clusters shaping k-means
// output on two obviously separated groups.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Two obvious clusters, near (0, 0) and near (10, 10).
  const cls::Series x = pluss::make_series(
      {0.0, 0.1, 0.2, 0.1, 10.0, 10.1, 9.9, 10.2}, {8}, f64).reshape({4, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  cls::ClusterFamilyOptions opts{};
  opts.clusters = 2;
  opts.restarts = 4;
  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(
      cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, opts});
  made.value()->learn(batch);
  const cls::StatusOr<pluss::Series> assignments = made.value()->map(batch);

  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";
  std::cout << "cluster assignments = " << assignments.value() << "\n";

  return 0;
}
