<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 364e09f477f8 -->

# `lymph::workflow::Policy` — `drop`, `require`, `forbid`

[Back to `lymph::workflow::Policy`](../Policy.md)

The three column rules. `drop` strips columns before modeling; `require` is a
raw-schema contract; `forbid` is a "this is leakage if it reaches here"
guard. None of them touch rows. Each is a small chainable setter that appends
to one of `Policy`'s name lists.

## Signature

```cpp
lymph::workflow::Policy& lymph::workflow::Policy::drop(std::vector<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::drop(std::initializer_list<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::require(std::vector<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::require(std::initializer_list<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::forbid(std::vector<std::string> names);
lymph::workflow::Policy& lymph::workflow::Policy::forbid(std::initializer_list<std::string> names);
```

## Parameters

`names` (`drop`)
: Columns removed from the frame before any model or prep stage sees it —
  IDs, leakage columns, audit-only fields. De-duplicated, order preserved.
  Appends to `drop_columns`.

`names` (`require`)
: Columns that must exist in the raw frame. Checked *before* `drop` is
  applied, so you can require a column and drop it in the same `Policy`.
  Appends to `require_columns`.

`names` (`forbid`)
: Columns that must *not* be in the raw frame — unless the same name is also
  in `drop_columns`, which reads as "yes, it is here on purpose, strip it".
  Appends to `forbid_columns`.

## Returns

The same `Policy` by reference, so the setters chain
(`policy.drop({...}).require({...})`).

## Errors

Raised by [`apply(frame)`](apply.md), not by the setters:

- a `require` column missing from the frame →
  `"missing required raw column \"<name>\""`.
- a `forbid` column present and not also dropped →
  `"forbidden raw column \"<name>\" is present; drop it explicitly if it is
  intentionally audit-only"`.

## Practical Examples

### Example 1

`drop({"customer_id"})` — the column count falls, the row count does not.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"customer_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106}, {6}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2}, {6}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1}, {6}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  const DataFrame admitted = policy.apply(raw);
  std::cout << "raw_features      = " << raw.nfeatures()
            << "  raw_rows      = " << raw.ncases() << "\n";
  std::cout << "admitted_features = " << admitted.nfeatures()
            << "  admitted_rows = " << admitted.ncases() << "\n";

  return 0;
}
```

Output

```text
raw_features      = 4  raw_rows      = 6
admitted_features = 3  admitted_rows = 6
```

### Example 2

`require({...})` passes silently when the columns are present, and fails
loudly — naming the column — when one is not.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2}, {6}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1}, {6}, f64))},
  });

  workflow::Policy ok{};
  ok.require({"income", "balance", "default"});
  std::cout << "require(present columns): admitted_rows=" << ok.apply(raw).ncases() << "\n";

  workflow::Policy missing{};
  missing.require({"income", "does_not_exist"});
  try {
    (void)missing.apply(raw);
    std::cout << "require(missing): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "require(missing) threw: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
require(present columns): admitted_rows=6
require(missing) threw: lymph::workflow::Policy::apply: missing required raw column "does_not_exist".
```

### Example 3

`forbid({"future_outcome"})` throws when the column is present — unless the
same `Policy` also `drop`s it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2}, {6}, f64))},
      {"future_outcome", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1}, {6}, f64))},
  });

  workflow::Policy strict{};
  strict.forbid({"future_outcome"});
  try {
    (void)strict.apply(raw);
    std::cout << "forbid alone: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "forbid alone threw: " << e.what() << "\n";
  }

  workflow::Policy acknowledged{};
  acknowledged.forbid({"future_outcome"}).drop({"future_outcome"});
  const DataFrame admitted = acknowledged.apply(raw);
  std::cout << "forbid + drop: admitted_features=" << admitted.nfeatures() << "\n";

  return 0;
}
```

Output

```text
forbid alone threw: lymph::workflow::Policy::apply: forbidden raw column "future_outcome" is present; drop it explicitly if it is intentionally audit-only.
forbid + drop: admitted_features=2
```

## See Also

- [`rows`](rows.md) — the row-rule counterpart (`reject` / `allow` /
  `reject_any_missing`).
- [`apply`](apply.md) — the method that runs `require` → `forbid` → `drop`
  in order.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply.html">
    <span class="label">Previous</span>
    <span class="title">apply</span>
  </a>
  <a class="next-link" href="rows.html">
    <span class="label">Next</span>
    <span class="title">rows</span>
  </a>
</div>
<!-- apiref:nav:end -->
