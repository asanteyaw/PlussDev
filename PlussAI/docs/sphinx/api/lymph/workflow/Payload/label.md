<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 39e3ec5cb0d3 -->

# `lymph::workflow::Payload::label`

[Back to `lymph::workflow::Payload`](../Payload.md)

`label` attaches the supervised target to a `Payload`. It has two spellings —
a column name for a frame-backed `Payload`, or an already-separate `Series`
for the case where the label does not live in the frame. Supervision is one
role: the later `.label(...)` call wins, and the two query methods stay
mutually exclusive.

## Signature

```cpp
lymph::workflow::Payload& lymph::workflow::Payload::label(std::string feature_name);
lymph::workflow::Payload& lymph::workflow::Payload::label(pluss::Series y);

// The queries that report which spelling is active:
bool lymph::workflow::Payload::has_label() const;          // true after label(Series)
bool lymph::workflow::Payload::has_label_feature() const;   // true after label(name)
```

## Parameters

`std::string feature_name`
: The single column in the backing frame to fit against. Excluded from the
  feature set even if `features(...)` does not name it. Works with every
  raw-frame `Workflow` verb.

`pluss::Series y`
: An already-separate label vector, one value per row. The form to use for a
  `FeatureBatch`-only `Payload` (no frame), or when the label is computed
  rather than stored. Its length must match the feature row count.

## Returns

The same `Payload` by reference, so calls chain. Attaching by name clears any
`Series` label previously attached, and vice versa — `has_label()` and
`has_label_feature()` are never both `true`.

## Errors

- `label` itself does not validate — a name that is not in the frame, or a
  `Series` of the wrong length, surfaces later, when
  [`labeled_batch()`](batch.md) or a `Workflow` verb materializes the batch.
- Calling [`labeled_batch()`](batch.md) on a `Payload` with no label role at
  all throws `"payload has no labels"`.

## Practical Examples

### Example 1

`label(name)` on a frame-backed `Payload` — the everyday form. It flows
straight into any `Workflow` verb.

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
      {"default", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label("default");

  std::cout << "has_label()         = " << std::boolalpha << payload.has_label() << "\n";
  std::cout << "has_label_feature() = " << payload.has_label_feature() << "\n";

  const lymph::LabeledBatch batch = payload.labeled_batch();
  std::cout << "labeled_batch: rows=" << batch.features.sample_count()
            << " features=" << batch.features.feature_count() << "\n";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(payload);
  std::cout << "trained.trained()   = " << trained.trained() << "\n";

  return 0;
}
```

Output

```text
has_label()         = false
has_label_feature() = true
labeled_batch: rows=6 features=2
trained.trained()   = true
```

### Example 2

`label(Series)` — the label supplied as a vector. Here the frame holds only
features; the label rides alongside.

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
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);

  const workflow::Payload payload =
      workflow::payload::make(raw).features({"income", "balance"}).label(y);

  std::cout << "has_label()         = " << std::boolalpha << payload.has_label() << "\n";
  std::cout << "has_label_feature() = " << payload.has_label_feature() << "\n";

  const lymph::LabeledBatch batch = payload.labeled_batch();
  std::cout << "labeled_batch: rows=" << batch.features.sample_count()
            << " features=" << batch.features.feature_count() << "\n";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(payload);
  std::cout << "pred = " << trained.infer(payload) << "\n";

  return 0;
}
```

Output

```text
has_label()         = true
has_label_feature() = false
labeled_batch: rows=6 features=2
pred = pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 0, 0, 1, 1, 1]
```

### Example 3

The last-call-wins rule, and the no-label error. Calling both spellings
leaves only the last one active; a `Payload` with no label at all cannot
materialize a `LabeledBatch`.

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
      {"default", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });
  const pluss::Series y = pluss::make_series({1, 1, 1, 0, 0, 0}, {6}, f64);

  const workflow::Payload series_wins =
      workflow::payload::make(raw).features({"income", "balance"}).label("default").label(y);
  std::cout << "name then Series: has_label()=" << std::boolalpha << series_wins.has_label()
            << " has_label_feature()=" << series_wins.has_label_feature() << "\n";

  const workflow::Payload name_wins =
      workflow::payload::make(raw).features({"income", "balance"}).label(y).label("default");
  std::cout << "Series then name: has_label()=" << name_wins.has_label()
            << " has_label_feature()=" << name_wins.has_label_feature() << "\n";

  const workflow::Payload unlabeled =
      workflow::payload::make(raw).features({"income", "balance"});
  try {
    (void)unlabeled.labeled_batch();
    std::cout << "labeled_batch(): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "labeled_batch() threw: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
name then Series: has_label()=true has_label_feature()=false
Series then name: has_label()=false has_label_feature()=true
labeled_batch() threw: lymph::workflow::Payload::labeled_batch: payload has no labels.
```

## See Also

- [`weights`](weights.md) — the same name-vs-`Series` split for per-row
  training weights.
- [`batch`](batch.md) — `labeled_batch()` reads the label back out.
- [`roles`](roles.md) — `features(...)`, which decides the columns the label
  is excluded from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batch.html">
    <span class="label">Previous</span>
    <span class="title">batch</span>
  </a>
  <a class="next-link" href="roles.html">
    <span class="label">Next</span>
    <span class="title">roles</span>
  </a>
</div>
<!-- apiref:nav:end -->
