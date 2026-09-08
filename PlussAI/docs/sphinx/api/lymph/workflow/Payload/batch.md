<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ec79b891c161 -->

# `lymph::workflow::Payload` — `feature_batch`, `labeled_batch`, `with_frame`

[Back to `lymph::workflow::Payload`](../Payload.md)

These three methods turn a `Payload` into the batch a model actually
consumes. Every `Workflow` verb calls one of them internally;
`feature_batch()` and `labeled_batch()` are also the way to inspect exactly
what a `Payload` resolves to. `with_frame()` swaps the backing frame while
keeping every attached role.

## Signature

```cpp
lymph::FeatureBatch lymph::workflow::Payload::feature_batch() const;
lymph::LabeledBatch lymph::workflow::Payload::labeled_batch() const;
lymph::workflow::Payload lymph::workflow::Payload::with_frame(pluss::table::DataFrame frame) const;
```

## Parameters

`feature_batch()` / `labeled_batch()` take none.

`pluss::table::DataFrame frame` (`with_frame`)
: The replacement frame. Every role and option on the `Payload` is copied
  onto the result unchanged; only the data is different.

## Returns

`feature_batch()`
: A `FeatureBatch` — the feature columns only, in `features(...)` order (or
  every non-label / non-weight column if unset). Available from any shape
  that holds data; needs no label role.

`labeled_batch()`
: A validated `LabeledBatch` — features + label + any resolved weights.

`with_frame(frame)`
: A new `Payload` (the receiver is untouched) backed by `frame`, roles and
  options preserved.

## Errors

- `feature_batch()` throws if the `Payload` holds neither a frame nor a
  batch.
- `labeled_batch()` throws `"payload has no labels"` if no label role was
  attached, and surfaces any deferred problem here — an unhandled `Note`
  column, a bad feature name, a length mismatch.

## Practical Examples

### Example 1

`feature_batch()` from a frame-backed `Payload` — only the named feature
columns come through.

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
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6}, {5}, f64))},
      {"balance", Thread(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9}, {5}, f64))},
      {"note_col", Thread(pluss::make_series({9.0, 9.0, 9.0, 9.0, 9.0}, {5}, f64))},
  });

  const workflow::Payload payload =
      workflow::payload::make(raw).features({"income", "balance"});

  const lymph::FeatureBatch fb = payload.feature_batch();
  std::cout << "feature_batch: rows=" << fb.sample_count()
            << " cols=" << fb.feature_count() << " (note_col excluded)\n";

  return 0;
}
```

Output

```text
feature_batch: rows=5 cols=2 (note_col excluded)
```

### Example 2

`labeled_batch()` — features, label, and weights, all carried through.

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
      {"w", Thread(pluss::make_series({1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label("default")
                                        .weights("w");

  const lymph::LabeledBatch lb = payload.labeled_batch();
  std::cout << "labeled_batch: rows=" << lb.features.sample_count()
            << " features=" << lb.features.feature_count()
            << " has_weights=" << std::boolalpha << lb.has_weights() << "\n";

  return 0;
}
```

Output

```text
labeled_batch: rows=6 features=2 has_weights=true
```

### Example 3

`with_frame(frame)` returns a copy backed by a different frame; the label
role rides along.

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

  const DataFrame six({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"balance", Thread(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });
  const DataFrame three({
      {"income", Thread(pluss::make_series({1.0, 2.0, 3.0}, {3}, f64))},
      {"balance", Thread(pluss::make_series({0.6, 1.5, 2.2}, {3}, f64))},
      {"default", Thread(pluss::make_series({0, 1, 1}, {3}, f64))},
  });

  const workflow::Payload original =
      workflow::payload::make(six).features({"income", "balance"}).label("default");
  const workflow::Payload swapped = original.with_frame(three);

  std::cout << "original labeled_batch rows = " << original.labeled_batch().features.sample_count() << "\n";
  std::cout << "swapped  labeled_batch rows = " << swapped.labeled_batch().features.sample_count() << "\n";
  std::cout << "swapped  has_label_feature  = " << std::boolalpha << swapped.has_label_feature()
            << " (role preserved)\n";

  return 0;
}
```

Output

```text
original labeled_batch rows = 6
swapped  labeled_batch rows = 3
swapped  has_label_feature  = true (role preserved)
```

## See Also

- [`label`](label.md) — the role `labeled_batch()` requires.
- [`weights`](weights.md) — resolved into `labeled_batch().sample_weights`.
- [`Workflow::learn`](../Workflow/learn.md) — calls `labeled_batch()` /
  `feature_batch()` internally on a `Payload` argument.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="label.html">
    <span class="label">Next</span>
    <span class="title">label</span>
  </a>
</div>
<!-- apiref:nav:end -->
