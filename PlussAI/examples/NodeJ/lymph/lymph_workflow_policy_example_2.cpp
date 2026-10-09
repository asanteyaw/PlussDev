#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <limits>

// Exercises the Policy setters Example 1 does not: forbid, reject_missing
// (row-level), allow_missing (exempting a column from legacy filter_nulls),
// reject_any_missing (the one-call shorthand), and empty().
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

  // forbid(...): a column present and not also dropped is a hard error.
  workflow::Policy forbidding{};
  forbidding.forbid({"status"});
  try {
    (void)forbidding.apply(raw);
    std::cout << "forbid: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "forbid threw: " << e.what() << "\n";
  }

  // reject_missing(...): rows null in a named column are dropped -- narrow,
  // explicit row admission.
  workflow::Policy row_reject{};
  row_reject.reject({"income"});
  const DataFrame after_reject = row_reject.apply(raw);
  std::cout << "reject({\"income\"}): raw_rows=" << raw.ncases()
            << " admitted_rows=" << after_reject.ncases() << " (1 NaN row in income dropped)\n";

  // reject_any_missing(...): the one-call shorthand for "drop a row if any
  // of these named columns is null" -- equivalent to setting
  // null_filter_columns + drop_only_if_all_null=false + filter_nulls=true
  // by hand.
  workflow::Policy shorthand{};
  shorthand.reject_any_missing({"income", "balance"});
  const DataFrame after_shorthand = shorthand.apply(raw);
  std::cout << "reject_any_missing({\"income\",\"balance\"}): admitted_rows="
            << after_shorthand.ncases() << " (2 rows have a NaN in either column)\n";

  // allow_missing(...): exempts a column from the broad legacy filter_nulls
  // sweep, e.g. because an imputer stage owns that column's missing values.
  workflow::Policy legacy{};
  legacy.allow({"income"});
  legacy.filter_nulls = true;
  const DataFrame after_legacy = legacy.apply(raw);
  std::cout << "filter_nulls=true, allow({\"income\"}): admitted_rows="
            << after_legacy.ncases() << " (only balance's NaN row is dropped)\n";

  std::cout << "Policy{}.empty() = " << std::boolalpha << workflow::Policy{}.empty() << "\n";
  std::cout << "row_reject.empty() = " << row_reject.empty() << "\n";

  return 0;
}
