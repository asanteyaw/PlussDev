// estimators::TransformerBase example 3 -- map before learn fails.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Decompose,
                                           cls::DecomposeFamilyOptions{}});

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({1, 2});
  const cls::StatusOr<pluss::Series> mapped = made.value()->map(batch);
  std::cout << "ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "message = \"" << mapped.status().message << "\"\n";

  return 0;
}
