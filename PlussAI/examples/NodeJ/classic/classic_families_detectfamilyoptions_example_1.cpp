// families::DetectFamilyOptions example 1 -- isolation.contamination set
// explicitly, flagging a single far outlier.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 9 points clustered near 0, one far outlier.
  const cls::Series x = pluss::make_series(
      {0.0, 0.1, -0.1, 0.2, -0.2, 0.05, -0.05, 0.15, -0.15, 20.0}, {10}, f64).reshape({10, 1});
  cls::FeatureBatch batch{};
  batch.x = x;

  cls::DetectFamilyOptions opts{};
  opts.method = cls::DetectMethod::Isolation;
  opts.isolation.contamination_auto = false;
  opts.isolation.contamination = 0.1;  // expect ~10% outliers, matching 1-in-10 here

  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(
      cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Detect, opts});
  made.value()->learn(batch);
  const cls::StatusOr<pluss::Series> scores = made.value()->map(batch);

  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";
  std::cout << "scores = " << scores.value() << "\n";

  return 0;
}
