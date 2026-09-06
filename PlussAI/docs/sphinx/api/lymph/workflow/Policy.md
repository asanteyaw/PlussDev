<!-- pluss-hand-authored -->

# `lymph::workflow::Policy`

[Back to `lymph::workflow::Workflow`](Workflow.md)

`Policy` is a declarative, replayable raw-frame admission and schema contract
— governance, not feature engineering. It decides which raw columns may enter
a `Workflow` and which rows are admissible before any prep stage or model
sees the data. It does not scale, encode, or impute — those are `Model`/prep
stage concerns, attached through [`Workflow::on`](Workflow/on.md) or
`Model::append`. Every raw-frame `Workflow` verb applies its held `Policy`
first, via `Policy::apply(frame)`.

## Scope And Alias

`lymph::workflow::Policy` (implementation home:
`lymph::workflow::policy::Policy`). No umbrella `lymph::` re-export — write
`workflow::Policy`, or bring in `namespace workflow =
pluss::ai::lymph::workflow;`.

## Construction

```cpp
lymph::workflow::Policy lymph::workflow::policy::make(lymph::workflow::Policy options = {});
```

`Policy` is an aggregate-friendly value type — `Policy{}` (default-admits
everything) or `Policy policy; policy.drop({...}).require({...});` are both
normal. `policy::make(options)` exists for symmetry with the other family
factories; it is an identity pass-through.

## State

`std::vector<std::string> drop_columns`
: Columns stripped before modeling — IDs, leakage columns, audit-only fields.

`std::vector<std::string> require_columns`
: Columns that must be present *before* any drop is applied — a raw schema
  contract for labels, time keys, or features a later stage needs.

`std::vector<std::string> forbid_columns`
: Columns that must not appear in the raw frame at all, unless also listed in
  `drop_columns` — a "this is leakage if it reaches here" check.

`std::vector<std::string> reject_missing`
: Rows with a null in any of these named columns are dropped. The preferred,
  explicit row-admission surface — narrow on purpose, so feature columns
  meant for an imputer are not silently thinned here.

`std::vector<std::string> allow_missing` / `bool filter_nulls` / `std::vector<std::string> null_filter_columns` / `bool drop_only_if_all_null`
: Legacy broad row filtering (`DataFrame::dropnull` semantics). Prefer
  `reject_missing`/`reject_any_missing(...)` for new code — these fields stay
  for the caller that already leans on frame-wide null filtering.

`bool empty() const`
: True when every field above is at its default (no drop, no require, no
  forbid, no reject, `filter_nulls` false) — a genuine no-op `Policy`.

  Effect (Example 2) — `Policy{}.empty()=true`; a `Policy` with even one
  `reject(...)` name added has `empty()=false`.

## Configuration

`Policy& drop(std::vector<std::string> | std::initializer_list<std::string> names)`
: Adds to `drop_columns` (de-duplicated, order preserved).

  Effect (Example 1) — `.drop({"customer_id"})` on a 4-feature frame gives
  `apply(raw).nfeatures()=3`; the row count is unchanged (`drop` never
  removes rows, only columns).

`Policy& require(...)`
: Appends to `require_columns` — checked before any drop is applied.

  Effect (Example 1) — `.require({"income", "balance", "does_not_exist"})`
  makes `apply(...)` throw `"missing required raw column
  \"does_not_exist\""` before touching the frame at all.

`Policy& forbid(...)`
: Appends to `forbid_columns` — a column present and not also `drop`-ped is
  a hard error.

  Effect (Example 2) — `.forbid({"status"})` alone (no matching `drop`)
  makes `apply(...)` throw `"forbidden raw column \"status\" is present;
  drop it explicitly if it is intentionally audit-only"`.

`Policy& reject(...)`
: Appends to `reject_missing` — rows null in any named column are dropped.
  Narrow on purpose: only the columns you name are checked.

  Effect (Example 2) — on a 4-row frame with one `NaN` in `income`,
  `.reject({"income"})` gives `apply(raw).ncases()=3`.

`Policy& allow(...)`
: Appends to `allow_missing` — exempts named columns from the broad legacy
  `filter_nulls` sweep below (e.g. because an imputer stage owns that
  column's missing values, so `Policy` should not thin rows over it).

  Effect (Example 2) — with `filter_nulls=true` and no other rows/columns
  named, `.allow({"income"})` on the same frame gives
  `apply(raw).ncases()=3` — only the row null in the *other* column
  (`balance`) is dropped; `income`'s own null row survives because it is
  exempted.

`Policy& reject_any_missing(std::vector<std::string> | std::initializer_list<std::string> names)`
: Sets `null_filter_columns = names`, `drop_only_if_all_null = false`,
  `filter_nulls = true` in one call — the common "drop a row if any of these
  named columns is null" shape, without touching the legacy fields by hand.

  Effect (Example 2) — `.reject_any_missing({"income", "balance"})` on the
  same frame gives `apply(raw).ncases()=2`: the row null in `income` and the
  (different) row null in `balance` are both dropped.

## Methods

`pluss::table::DataFrame apply(const pluss::table::DataFrame& frame) const`
: Validates `frame` against `require_columns`/`drop_columns`/`reject_missing`/
  `allow_missing`/`null_filter_columns` (throws naming the first missing
  column), then checks `forbid_columns` (throws naming the first forbidden
  column still present and not also dropped), then applies `drop_columns`,
  `reject_missing` (row-level), and legacy `filter_nulls` row filtering in
  that order. This is the only method `Policy` has beyond the setters above —
  `Workflow` calls it once per raw-frame verb, before its held `Model` sees
  the frame.

## Practical Examples

### Example 1

This example admits a frame by dropping an ID column and requiring the rest,
then shows `require(...)` failing loudly (rather than silently) when a
required column is absent.

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
  policy.require({"income", "balance", "default"});

  const DataFrame admitted = policy.apply(raw);
  std::cout << "== policy_apply ==\n";
  std::cout << "raw_rows=" << raw.ncases() << " admitted_rows=" << admitted.ncases() << "\n";
  std::cout << "raw_features=" << raw.nfeatures()
            << " admitted_features=" << admitted.nfeatures() << "\n";

  workflow::Policy strict{};
  strict.require({"income", "balance", "does_not_exist"});
  try {
    (void)strict.apply(raw);
    std::cout << "strict.apply: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "strict.apply threw: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
== policy_apply ==
raw_rows=6 admitted_rows=6
raw_features=4 admitted_features=3
strict.apply threw: lymph::workflow::Policy::apply: missing required raw column "does_not_exist".
```

### Example 2

The setters Example 1 does not exercise: `forbid`, `reject` (row-level),
`allow` (exempting a column from the legacy sweep), and the
`reject_any_missing` shorthand. Every "Effect" line quoted next to a setter
above comes from this program's output.

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
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const double nan_val = std::numeric_limits<double>::quiet_NaN();
  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, nan_val, 1.8, 2.2}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, nan_val, 0.9}, {4}, f64))},
      {"status", Thread(pluss::make_series({0.0, 1.0, 0.0, 1.0}, {4}, f64))},
  });

  workflow::Policy forbidding{};
  forbidding.forbid({"status"});
  try {
    (void)forbidding.apply(raw);
    std::cout << "forbid: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "forbid threw: " << e.what() << "\n";
  }

  workflow::Policy row_reject{};
  row_reject.reject({"income"});
  const DataFrame after_reject = row_reject.apply(raw);
  std::cout << "reject({\"income\"}): raw_rows=" << raw.ncases()
            << " admitted_rows=" << after_reject.ncases() << " (1 NaN row in income dropped)\n";

  workflow::Policy shorthand{};
  shorthand.reject_any_missing({"income", "balance"});
  const DataFrame after_shorthand = shorthand.apply(raw);
  std::cout << "reject_any_missing({\"income\",\"balance\"}): admitted_rows="
            << after_shorthand.ncases() << " (2 rows have a NaN in either column)\n";

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
```

Output

```text
forbid threw: lymph::workflow::Policy::apply: forbidden raw column "status" is present; drop it explicitly if it is intentionally audit-only.
reject({"income"}): raw_rows=4 admitted_rows=3 (1 NaN row in income dropped)
reject_any_missing({"income","balance"}): admitted_rows=2 (2 rows have a NaN in either column)
filter_nulls=true, allow({"income"}): admitted_rows=3 (only balance's NaN row is dropped)
Policy{}.empty() = true
row_reject.empty() = false
```

## Changelog

- **0.1** — `Policy` = drop/require/forbid/reject_missing/allow_missing +
  legacy `filter_nulls` fields, `apply(frame)`.

## See Also

- [`Workflow`](Workflow.md) — applies `Policy` on every raw-frame verb.
- [`Payload`](Payload.md) — a `Payload`'s frame also goes through `Policy`.
- [`Workflow::save`](Workflow/save.md) — persists `Policy` as a bundle
  sidecar file.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
