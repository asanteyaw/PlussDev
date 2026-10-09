#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <limits>

// allow(names) with the legacy filter_nulls sweep: filter_nulls=true drops a
// row if any remaining column is null; allow(names) exempts those columns
// from the sweep (an imputer stage owns their missing values). Prefer
// reject()/reject_any_missing() for new code -- this shows the legacy path
// for a caller that already leans on frame-wide filtering.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const double nan_val = std::numeric_limits<double>::quiet_NaN();
  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, nan_val, 1.8, 2.2}, {4}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, nan_val, 0.9}, {4}, f64))},
  });

  // Sweep every column: both NaN rows go.
  workflow::Policy broad{};
  broad.filter_nulls = true;
  std::cout << "filter_nulls only:            admitted_rows=" << broad.apply(raw).ncases() << "\n";

  // Exempt income: only balance's NaN row is dropped.
  workflow::Policy exempted{};
  exempted.filter_nulls = true;
  exempted.allow({"income"});
  std::cout << "filter_nulls + allow(income): admitted_rows=" << exempted.apply(raw).ncases() << "\n";

  return 0;
}
