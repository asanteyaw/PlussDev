// families::DecomposeFamilyOptions example 1 -- components reducing a
// 3-feature input to its top 1 or 2 components.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Three correlated features -- most of the variance lives in one direction.
  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 1.0, 2.0, 4.0, 2.1, 3.0, 6.0, 3.2, 4.0, 8.0, 3.9},
      {12}, f64).reshape({4, 3});
  cls::FeatureBatch batch{};
  batch.x = x;

  auto fit_and_map = [&](std::int64_t components) {
    cls::DecomposeFamilyOptions opts{};
    opts.components = components;
    cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(
        cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Decompose, opts});
    made.value()->learn(batch);
    return made.value()->map(batch).value();
  };

  std::cout << "components = 1 ->\n" << fit_and_map(1) << "\n";
  std::cout << "components = 2 ->\n" << fit_and_map(2) << "\n";

  return 0;
}
