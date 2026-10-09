<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bdaf8f3cfc3a -->

# `lymph::workflow::Payload::weights`

[Back to `lymph::workflow::Payload`](../Payload.md)

`weights` attaches per-row training weights — a value each observation's
contribution to the loss is multiplied by. Like [`label`](label.md), it takes
either a column name or an already-separate `Series`. Unlike `label`, the two
routes are *not* mutually exclusive query-wise: three `has_*` methods report
exactly where the weights came from, and a frame-backed `Payload` must use
the *named* form so `Policy` row filtering keeps the weights aligned.

## Signature

```cpp
lymph::workflow::Payload& lymph::workflow::Payload::weights(std::string feature_name);
lymph::workflow::Payload& lymph::workflow::Payload::weights(pluss::Series sample_weights);

bool lymph::workflow::Payload::has_weights() const;           // any source
bool lymph::workflow::Payload::has_weight_feature() const;    // true after weights(name)
bool lymph::workflow::Payload::has_external_weights() const;  // true after weights(Series)
```

## What It Does

`weights` attaches a per-row weight that scales each row's contribution to the loss. It takes a column name or a separate series. A frame-backed payload must use the column form, so that row filtering keeps the weights aligned with the rows.

## Parameters

`std::string feature_name`
: A column supplying the weights. It is not treated as a feature. This is the
  form to use for a frame-backed `Payload`: because `Policy` can drop rows,
  the weights must travel *in the frame* so they are filtered in lockstep.

`pluss::Series sample_weights`
: An already-separate weight vector, one value per row. Use it for a
  `FeatureBatch`-only `Payload`, or a bare-batch path where `Policy` row
  filtering is not in play.

## Returns

The same `Payload` by reference. `has_weights()` is `true` if weights come
from *any* source (a named column, a `Series`, or a `LabeledBatch` that
already carried them); `has_weight_feature()` and `has_external_weights()`
distinguish the two attach routes.

## Errors

- Handing a `Workflow` verb a frame-backed `Payload` that carries a `Series`
  weight but *no* named weight column throws
  `"DataFrame-backed Payload with Series weights must use weights(\"column\")
  so policy/filtering keeps rows aligned"` (Example 3).

## Practical Examples

### Example 1

`weights(name)` — the frame-backed form. The named column is read as weights
and kept out of the feature set; the materialized `LabeledBatch` carries
them.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2}, {6}, f64))},
      {"row_weight", Slate(pluss::make_series({1.0, 1.0, 1.0, 3.0, 3.0, 3.0}, {6}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label("default")
                                        .weights("row_weight");

  std::cout << "has_weights()          = " << std::boolalpha << payload.has_weights() << "\n";
  std::cout << "has_weight_feature()   = " << payload.has_weight_feature() << "\n";
  std::cout << "has_external_weights() = " << payload.has_external_weights() << "\n";

  const lymph::LabeledBatch batch = payload.labeled_batch();
  std::cout << "labeled_batch().has_weights() = " << batch.has_weights() << "\n";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(payload);
  std::cout << "trained.trained() = " << trained.trained() << "\n";

  return 0;
}
```

Output

```text
has_weights()          = true
has_weight_feature()   = true
has_external_weights() = false
labeled_batch().has_weights() = true
trained.trained() = true
```

### Example 2

`weights(Series)` — the weight vector supplied alongside a `Series` label, no
weight column in the frame. `has_external_weights()` is the one that flips.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2}, {6}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const pluss::Series w = pluss::make_series({1.0, 1.0, 1.0, 3.0, 3.0, 3.0}, {6}, f64);

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label(y)
                                        .weights(w);

  std::cout << "has_weights()          = " << std::boolalpha << payload.has_weights() << "\n";
  std::cout << "has_weight_feature()   = " << payload.has_weight_feature() << "\n";
  std::cout << "has_external_weights() = " << payload.has_external_weights() << "\n";

  const lymph::LabeledBatch batch = payload.labeled_batch();
  std::cout << "labeled_batch().has_weights() = " << batch.has_weights() << "\n";

  return 0;
}
```

Output

```text
has_weights()          = true
has_weight_feature()   = false
has_external_weights() = true
labeled_batch().has_weights() = true
```

### Example 3

The alignment guard: a frame-backed `Payload` + a `Series` weight + no named
column. `Policy` could drop rows from the frame and leave the weight vector
the wrong length, so `Workflow` rejects it up front.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2}, {6}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });
  const pluss::Series w = pluss::make_series({1.0, 1.0, 1.0, 3.0, 3.0, 3.0}, {6}, f64);

  const workflow::Payload bad = workflow::payload::make(raw)
                                    .features({"income", "balance"})
                                    .label("default")
                                    .weights(w);

  try {
    (void)workflow::make(lymph::models::logistic_regression()).learn(bad);
    std::cout << "learn(bad): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "learn(bad) threw: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
learn(bad) threw: lymph::workflow::Workflow::learn_(payload): DataFrame-backed Payload with Series weights must use weights("column") so policy/filtering keeps rows aligned.
```

## See Also

- [`label`](label.md) — the same name-vs-`Series` split for supervision.
- [`Policy`](../Policy.md) — the row filtering that makes the named form
  mandatory for frame-backed payloads.
- [`batch`](batch.md) — `labeled_batch()` carries the resolved weights.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="time.html">
    <span class="label">Previous</span>
    <span class="title">time</span>
  </a>
</div>
<!-- apiref:nav:end -->
