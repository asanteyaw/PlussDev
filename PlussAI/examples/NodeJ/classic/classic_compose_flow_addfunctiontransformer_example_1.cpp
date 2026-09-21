// compose::Flow::add_function_transformer example 1 -- wrapping a plain
// Series -> Series callable as an opaque stage, then running it through
// Flow::map.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FunctionTransformer doubler(
      [](const pluss::Series& x) -> cls::StatusOr<pluss::Series> { return cls::StatusOr<pluss::Series>(x.mul(2.0)); });

  cls::Flow flow{};
  const cls::Status s = flow.add_function_transformer(std::move(doubler));
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  std::cout << "has_opaque_transforms() = " << flow.has_opaque_transforms() << "\n";

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series mapped = flow.map(batch).value();
  std::cout << "mapped = " << mapped << "\n";

  return 0;
}
