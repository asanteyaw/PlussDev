// estimators::TransformerBase example 2 -- needs_target() and
// learn_supervised, verified for a Cluster-family transformer.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::ClusterFamilyOptions opts{};
  opts.clusters = 2;
  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, opts});
  std::cout << "needs_target() = " << std::boolalpha << made.value()->needs_target() << "\n";

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({0.0, 0.1, 10.0, 10.1}, {4}, f64).reshape({4, 1});
  const pluss::Series unused_target = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const cls::Status learned = made.value()->learn_supervised(batch, unused_target);
  std::cout << "learn_supervised ok = " << learned.ok() << "\n";
  std::cout << "trained()           = " << made.value()->trained() << "\n";

  return 0;
}
