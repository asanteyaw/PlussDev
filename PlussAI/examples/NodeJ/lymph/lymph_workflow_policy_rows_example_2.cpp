#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <limits>

// reject_any_missing(names): the one-call shorthand for "drop a row if ANY
// of these named columns is null". It sets null_filter_columns +
// drop_only_if_all_null=false + filter_nulls=true together, so you do not
// touch the legacy fields by hand.
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
      {"status", Slate(pluss::make_series({0.0, 1.0, 0.0, 1.0}, {4}, f64))},
  });

  workflow::Policy policy{};
  policy.reject_any_missing({"income", "balance"});
  const DataFrame admitted = policy.apply(raw);
  std::cout << "raw_rows      = " << raw.ncases() << "\n";
  std::cout << "admitted_rows = " << admitted.ncases()
            << " (row 2 null in income, row 3 null in balance -- both dropped)\n";

  return 0;
}
