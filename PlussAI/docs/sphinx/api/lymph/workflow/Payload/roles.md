<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8414c73da9dc -->

# `lymph::workflow::Payload` — `features`, `groups`, and data-conversion shortcuts

[Back to `lymph::workflow::Payload`](../Payload.md)

This page covers the `Payload` setters that are not big enough for their own
page: `features(...)` (which columns are features), `groups(...)` (the
grouping key for grouped cross-validation), and `note` / `layout` / `thread`
(shortcuts into the `DataFrameBatchOptions` data-conversion policy).

## Signature

```cpp
lymph::workflow::Payload& lymph::workflow::Payload::features(std::vector<std::string> names);
lymph::workflow::Payload& lymph::workflow::Payload::groups(pluss::Series groups);
lymph::workflow::Payload& lymph::workflow::Payload::note(lymph::DataNotePolicy policy);
lymph::workflow::Payload& lymph::workflow::Payload::layout(lymph::ObservationLayout layout);
lymph::workflow::Payload& lymph::workflow::Payload::thread(lymph::DataThreadPolicy policy);

bool lymph::workflow::Payload::has_groups() const;
const pluss::Series& lymph::workflow::Payload::groups() const;
```

## Parameters

`std::vector<std::string> names` (`features`)
: The exact feature columns, in order. Left empty, every column except the
  label and the weight column is used. Naming them makes the input contract
  explicit and stable if the frame later grows columns (Example 1).

`pluss::Series groups` (`groups`)
: A per-row entity key. `has_groups()` reflects the attachment;
  [`selection::validate`](../../selection/validate.md)'s grouped plan is the
  consumer — it keeps a single entity from straddling the train/test
  boundary (Example 2).

`lymph::DataNotePolicy policy` (`note`)
: How `Note` (text) columns among the features are handled when a batch is
  materialized. The default (`Reject`) refuses them; `Factorize` turns them
  into integer codes (Example 3). Shortcut for `data().note_policy`.

`lymph::ObservationLayout layout` (`layout`) · `lymph::DataThreadPolicy policy` (`thread`)
: Shortcuts for `data().layout` (whether observations run down rows or
  across columns) and `data().thread_policy` (how `Thread`-typed columns
  convert). Rarely set; the defaults are correct for a row-major frame.

## Returns

The same `Payload` by reference. Each setter writes one field of
`data()` — reachable directly through the mutable `data()` accessor too,
which is the escape hatch for a field with no dedicated setter.

## Errors

- None of these setters validate on the spot. A `features` name not in the
  frame, or a `Note` column with `note_policy = Reject`, surfaces when
  [`labeled_batch()`](batch.md) / [`feature_batch()`](batch.md) materializes
  (Example 3).

## Practical Examples

### Example 1

`features({...})` pins the feature set. Left implicit, an extra column is
pulled in.

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
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"balance", Thread(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2}, {6}, f64))},
      {"audit_note", Thread(pluss::make_series({9.0, 9.0, 9.0, 9.0, 9.0, 9.0}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  const workflow::Payload pinned = workflow::payload::make(raw)
                                       .features({"income", "balance"})
                                       .label("default");
  std::cout << "pinned  labeled_batch features = "
            << pinned.labeled_batch().features.feature_count() << "\n";

  const workflow::Payload implicit = workflow::payload::make(raw).label("default");
  std::cout << "implicit labeled_batch features = "
            << implicit.labeled_batch().features.feature_count() << "\n";

  return 0;
}
```

Output

```text
pinned  labeled_batch features = 2
implicit labeled_batch features = 3
```

### Example 2

`groups(Series)` attaches an entity key without touching the label or
feature roles.

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
      {"x1", Thread(pluss::make_series({1.0, 1.2, 3.0, 3.2, 5.0, 5.2, 7.0, 7.2}, {8}, f64))},
      {"x2", Thread(pluss::make_series({0.5, 0.7, 2.5, 2.7, 4.5, 4.7, 6.5, 6.7}, {8}, f64))},
      {"y",  Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });
  const pluss::Series groups = pluss::make_series({0, 0, 1, 1, 2, 2, 3, 3}, {8}, f64);

  const workflow::Payload without = workflow::payload::make(raw).features({"x1", "x2"}).label("y");
  const workflow::Payload with_groups =
      workflow::payload::make(raw).features({"x1", "x2"}).label("y").groups(groups);

  std::cout << "without.has_groups()     = " << std::boolalpha << without.has_groups() << "\n";
  std::cout << "with_groups.has_groups() = " << with_groups.has_groups() << "\n";
  std::cout << "with_groups.groups().numel() = " << with_groups.groups().numel() << "\n";

  const lymph::LabeledBatch batch = with_groups.labeled_batch();
  std::cout << "labeled_batch: rows=" << batch.features.sample_count()
            << " features=" << batch.features.feature_count() << "\n";

  return 0;
}
```

Output

```text
without.has_groups()     = false
with_groups.has_groups() = true
with_groups.groups().numel() = 8
labeled_batch: rows=8 features=2
```

### Example 3

`note(Factorize)` is what lets a `Note` (text) column reach the batch. The
default policy rejects it.

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
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"region", Thread(pluss::make_note(std::vector<std::string>{
          "n", "s", "n", "s", "e", "w"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  const workflow::Payload no_policy =
      workflow::payload::make(raw).features({"income", "region"}).label("default");
  try {
    (void)no_policy.labeled_batch();
    std::cout << "no policy: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "no policy threw: " << e.what() << "\n";
  }

  const workflow::Payload factorized = workflow::payload::make(raw)
                                           .features({"income", "region"})
                                           .label("default")
                                           .note(lymph::DataNotePolicy::Factorize);
  const lymph::LabeledBatch batch = factorized.labeled_batch();
  std::cout << "note(Factorize): labeled_batch features = "
            << batch.features.feature_count() << "\n";

  return 0;
}
```

Output

```text
no policy threw: lymph::workflow::Payload::labeled_batch: DataFrame feature 'region' is Note-backed and note_policy is Reject.
note(Factorize): labeled_batch features = 2
```

## See Also

- [`label`](label.md) / [`weights`](weights.md) — the roles `features(...)`
  excludes columns from.
- [`batch`](batch.md) — where an unhandled `Note` column or a bad feature
  name actually surfaces.
- [`selection::validate`](../../selection/validate.md) — the consumer of
  `groups(...)`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="label.html">
    <span class="label">Previous</span>
    <span class="title">label</span>
  </a>
  <a class="next-link" href="time.html">
    <span class="label">Next</span>
    <span class="title">time</span>
  </a>
</div>
<!-- apiref:nav:end -->
