<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fdb90d2f1235 -->

# `lymph::workflow::Policy` — `reject`, `allow`, `reject_any_missing`

[Back to `lymph::workflow::Policy`](../Policy.md)

The row rules. A row is dropped when it is missing a value in a column the
`Policy` cares about. `reject` is the preferred, narrow surface;
`reject_any_missing` is a one-call shorthand for the common case; `allow` and
the `filter_nulls` fields are the legacy broad-sweep path.

## Signature

```cpp
lymph::workflow::Policy& lymph::workflow::Policy::reject(std::vector<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::reject(std::initializer_list<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::allow(std::vector<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::allow(std::initializer_list<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::reject_any_missing(std::vector<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::reject_any_missing(std::initializer_list<std::string> names);

// The legacy fields reject_any_missing() sets for you:
bool         lymph::workflow::Policy::filter_nulls;
std::vector<std::string> lymph::workflow::Policy::null_filter_columns;
bool         lymph::workflow::Policy::drop_only_if_all_null;
```

## What It Does

The row rules of a policy. A row is dropped when it is missing a value in a column the policy names. `reject` is the narrow, preferred form, `reject_any_missing` is the shorthand for rejecting on any missing value, and `allow` with the null filter fields is the older broad path.

## Parameters

`names` (`reject`)
: Columns whose null rows are dropped. Narrow by design — only the columns
  you name are checked, so feature columns meant for an imputer are left
  untouched. Appends to `reject_missing`. This is the surface to reach for
  first (Example 1).

`names` (`reject_any_missing`)
: A one-call shorthand that sets `null_filter_columns = names`,
  `drop_only_if_all_null = false`, and `filter_nulls = true` together — "drop
  a row if *any* of these columns is null" (Example 2).

`names` (`allow`)
: Columns exempted from the broad `filter_nulls` sweep — their nulls do not
  cause a row to be dropped (Example 3). Only meaningful when
  `filter_nulls = true` and `null_filter_columns` is empty.

`filter_nulls` / `null_filter_columns` / `drop_only_if_all_null`
: The raw legacy fields (`DataFrame::dropnull` semantics). Prefer `reject` /
  `reject_any_missing` for new code; these stay for a caller who already
  leans on frame-wide null filtering.

## Returns

The same `Policy` by reference, so the setters chain.

## Errors

- A `reject` / `allow` / `null_filter_columns` name that is not in the frame
  makes [`apply(frame)`](apply.md) throw `"missing required raw column
  \"<name>\""`, the same as `require`.

## Practical Examples

### Example 1

`reject({"label"})` — only rows missing the label are dropped; a `NaN`
elsewhere survives.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <limits>

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
      {"label", Slate(pluss::make_series({0.0, nan_val, 1.0, 1.0, nan_val, 0.0}, {6}, f64))},
      {"income", Slate(pluss::make_series({1.0, 1.4, nan_val, 2.2, 3.8, 4.2}, {6}, f64))},
  });

  workflow::Policy policy{};
  policy.reject({"label"});
  const DataFrame admitted = policy.apply(raw);
  std::cout << "raw_rows      = " << raw.ncases() << "\n";
  std::cout << "admitted_rows = " << admitted.ncases() << " (2 null-label rows dropped)\n";

  return 0;
}
```

Output

```text
raw_rows      = 6
admitted_rows = 4 (2 null-label rows dropped)
```

### Example 2

`reject_any_missing({"income", "balance"})` — a row goes if *either* named
column is null.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <limits>

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
```

Output

```text
raw_rows      = 4
admitted_rows = 2 (row 2 null in income, row 3 null in balance -- both dropped)
```

### Example 3

`allow({"income"})` with the legacy `filter_nulls` sweep — `income`'s nulls
stop causing row drops, but every other column's still do.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <limits>

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

  workflow::Policy broad{};
  broad.filter_nulls = true;
  std::cout << "filter_nulls only:            admitted_rows=" << broad.apply(raw).ncases() << "\n";

  workflow::Policy exempted{};
  exempted.filter_nulls = true;
  exempted.allow({"income"});
  std::cout << "filter_nulls + allow(income): admitted_rows=" << exempted.apply(raw).ncases() << "\n";

  return 0;
}
```

Output

```text
filter_nulls only:            admitted_rows=2
filter_nulls + allow(income): admitted_rows=3
```

## See Also

- [`columns`](columns.md) — the column-rule counterpart.
- [`apply`](apply.md) — runs the row rules after `drop`, in a fixed order.
- [`Payload::note`](../Payload/roles.md) — feature-column missing values are
  usually an imputer stage's job, not `Policy`'s.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="columns.html">
    <span class="label">Previous</span>
    <span class="title">columns</span>
  </a>
</div>
<!-- apiref:nav:end -->
