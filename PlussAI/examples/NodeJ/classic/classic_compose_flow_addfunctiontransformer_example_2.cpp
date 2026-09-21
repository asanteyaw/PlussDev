// compose::Flow::add_function_transformer example 2 -- a callable that
// changes the sample count is rejected at map() time, not add time.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Drops the last row -- an invalid transform, since row count must be
  // preserved by default (FunctionTransformerOptions::preserve_sample_count).
  cls::prep::FunctionTransformer drops_a_row([](const pluss::Series& x) -> cls::StatusOr<pluss::Series> {
    return cls::StatusOr<pluss::Series>(x.slice(0, 0, 1));
  });

  cls::Flow flow{};
  const cls::Status add_status = flow.add_function_transformer(std::move(drops_a_row));
  std::cout << "add ok = " << std::boolalpha << add_status.ok() << "\n";

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const cls::StatusOr<pluss::Series> mapped = flow.map(batch);
  std::cout << "map ok = " << mapped.ok() << "\n";
  std::cout << "map message = \"" << mapped.status().message << "\"\n";

  return 0;
}
