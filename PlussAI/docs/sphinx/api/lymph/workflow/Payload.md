<!-- pluss-hand-authored -->

# `lymph::workflow::Payload`

[Back to `lymph::workflow::Workflow`](Workflow.md)

`Payload` is the typed data-and-role carrier for `Workflow`'s six data verbs —
[`learn`](Workflow/learn.md), [`map`](Workflow/map.md), [`infer`](Workflow/infer.md),
[`assess`](Workflow/assess.md), [`save`](Workflow/save.md) — for the case where a
`DataFrame` plus a plain `(x, y)` pair is not enough: supervised work that
also needs a weight role, a group role, or a time role attached explicitly by
name, without a parameter list that grows with every new role. Every `Payload`
verb overload on `Workflow` exists alongside its `(frame, DataFrameBatchOptions)`
and bare-batch siblings — `Payload` is a convenience, not the only way in.

## Scope And Alias

`lymph::workflow::Payload` (implementation home:
`lymph::workflow::payload::Payload`). No umbrella `lymph::` re-export — write
`workflow::Payload`, or bring in `namespace workflow =
pluss::ai::lymph::workflow;`.

## Construction

```cpp
lymph::workflow::Payload lymph::workflow::payload::make(
    pluss::table::DataFrame frame, lymph::DataFrameBatchOptions options = {});
lymph::workflow::Payload lymph::workflow::payload::make(lymph::FeatureBatch features);
lymph::workflow::Payload lymph::workflow::payload::make(lymph::LabeledBatch labeled);
```

Three starting shapes: a raw `DataFrame` (roles attached afterward via the
setters below), an already-built `FeatureBatch` (unsupervised/inference-only
use), or an already-built `LabeledBatch` (already has features + label +
optional weights — the setters below are mostly moot on this form).

## State

`bool has_frame() const` / `has_features() const` / `has_labeled() const`
: Which of the three constructed shapes this `Payload` holds.

`bool has_label() const` / `has_label_feature() const`
: `has_label()` is true after `.label(Series)`; `has_label_feature()` is true
  after `.label(std::string feature_name)`. A `Payload` never has both — the
  later call to `.label(...)` wins.

`bool has_weights() const` / `has_weight_feature() const` / `has_external_weights() const`
: `has_weights()` is true if weights come from any source (an explicit
  `Series`, a named column, or a `LabeledBatch` that already carries them).
  `has_weight_feature()`/`has_external_weights()` distinguish which.

`bool has_groups() const` / `has_time() const`
: Whether `.groups(...)` / any `.time(...)` overload has been called.

`const DataFrameBatchOptions& data() const` / `DataFrameBatchOptions& data()`
: The options record `.features(...)`/`.label(name)`/`.weights(name)`/
  `.layout(...)`/`.note(...)`/`.thread(...)` write into. Mutable access lets a
  caller reach in for a field `Payload` has no dedicated setter for.

`const pluss::table::DataFrame& frame() const` / `const FeatureBatch& features() const` / `const LabeledBatch& labeled() const`
: Throw if the `Payload` was not built from that shape.

`const Series& label() const` / `weights() const` / `groups() const` / `time() const`
: The externally-supplied `Series` for each role — `Series::undefined()` if
  that role was set by column name (or not set at all).

`std::string time_feature() const`
: The time column's name — from `.time(std::string)` directly, or from
  `TimeOptions::index` when `.time(TimeOptions)` was used. Empty if unset.

`const std::optional<TimeOptions>& time_options() const`
: Set only by `.time(TimeOptions)`; `std::nullopt` otherwise (including when
  `.time(std::string)` was used).

## Configuration

`Payload& features(std::vector<std::string> names)`
: Sets `data().feature_names`.

`Payload& label(std::string feature_name)` / `Payload& label(Series y)`
: Supervise by column name (works with any raw-frame verb) or by an
  already-separate `Series` (works with a `FeatureBatch`-only `Payload` too).

  Effect (Example 1) — `.label("default")` gives
  `has_label_feature()=true`; the same `Payload` built with `.label(y)`
  instead gives `has_label()=true`, `has_label_feature()=false`. The two
  accessors are mutually exclusive; the later `.label(...)` call wins.

`Payload& weights(std::string feature_name)` / `Payload& weights(Series sample_weights)`
: Same name-vs-`Series` split as `label`. A `Series`-weighted, frame-backed
  `Payload` must still name the column via `weights(feature_name)` for
  `Policy` row-filtering to stay aligned — `Workflow` throws otherwise.

  Effect (Example 2) — `.weights("sample_weight")` gives
  `has_weights()=true`, `has_weight_feature()=true`,
  `has_external_weights()=false`; `.weights(series)` instead gives
  `has_weights()=true`, `has_weight_feature()=false`,
  `has_external_weights()=true`.

`Payload& groups(Series groups)`
: Group keys for grouped cross-validation, read by
  [`selection::validate`](../selection/validate.md).

  Effect (Example 2) — `.groups(series)` gives `has_groups()=true`; nothing
  else on the `Payload` changes.

`Payload& time(std::string feature_name)` / `Payload& time(TimeOptions options)` / `Payload& time(Series values)`
: Attach a time role by column name, by a full
  [`TimeOptions`](TimeOptions.md) (validated immediately — throws on a
  negative `horizon`/`gap`/`lookback`, or an empty `index` when
  `reject_missing_index` is left at its default `true`), or by an
  already-separate `Series`.

  Effect (Example 2) — `.time(time_options)` (with `time_options.index =
  "observed_at"`, `horizon = steps(1)`) gives `time_feature()="observed_at"`,
  `time_options().has_value()=true`, `time_options()->horizon=1`. The
  simpler `.time("observed_at")` overload (Example 1) sets `time_feature()`
  the same way but leaves `time_options()` at `std::nullopt`.

`Payload& layout(ObservationLayout layout)` / `Payload& note(DataNotePolicy policy)` / `Payload& thread(DataThreadPolicy policy)`
: Shortcuts for `data().layout` / `data().note_policy` / `data().thread_policy`.
  `.note(DataNotePolicy::Factorize)` is what lets a `Note`-typed column
  (Example 1's `region`) reach `feature_batch()`/`labeled_batch()` at all —
  the default policy rejects `Note` columns outright.

## Reduction

`Payload with_frame(pluss::table::DataFrame frame) const`
: Returns a copy with a different backing frame, roles and options otherwise
  unchanged — used internally by `Workflow` once `Policy::apply(...)` has run.

`FeatureBatch feature_batch() const`
: Materializes a `FeatureBatch` from whichever shape this `Payload` holds.
  Throws if the `Payload` has neither a frame nor a batch.

  Effect (Example 2) — a `Payload` with 2 named features over 4 rows gives
  `feature_batch().sample_count()=4`, `feature_batch().feature_count()=2`.

`LabeledBatch labeled_batch() const`
: Materializes a `LabeledBatch`, validating it before returning. Throws if
  the `Payload` has no label role at all.

  Effect (Example 2) — the same `Payload`, weighted via a named column,
  gives `labeled_batch().features.sample_count()=4`,
  `labeled_batch().has_weights()=true`.

## Practical Examples

### Example 1

This example attaches feature, label, and time roles to a raw frame,
materializes the labeled batch, and prints the resulting state — then builds
a second `Payload` supervised by an already-separate label `Series` instead
of a column name, to contrast `has_label()` against `has_label_feature()`.

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
      {"customer_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106, 107, 108}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"region", Thread(pluss::make_note(std::vector<std::string>{
          "north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z",
      }))},
  });

  workflow::Payload payload = workflow::payload::make(raw)
                                  .features({"income", "balance", "region"})
                                  .label("default")
                                  .time("observed_at")
                                  .note(lymph::DataNotePolicy::Factorize);

  const lymph::LabeledBatch labeled = payload.labeled_batch();
  std::cout << "== payload state ==\n";
  std::cout << "has_frame=" << payload.has_frame()
            << " has_label_feature=" << payload.has_label_feature()
            << " has_time=" << payload.has_time()
            << " time_feature=" << payload.time_feature() << "\n";
  std::cout << "labeled_batch rows=" << labeled.features.sample_count()
            << " feature_width=" << labeled.features.feature_count() << "\n";

  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const workflow::Payload xy_payload =
      workflow::payload::make(raw).features({"income", "balance"}).label(y);
  std::cout << "== xy payload state ==\n";
  std::cout << "has_label=" << xy_payload.has_label()
            << " has_label_feature=" << xy_payload.has_label_feature() << "\n";

  return 0;
}
```

Output

```text
== payload state ==
has_frame=1 has_label_feature=1 has_time=1 time_feature=observed_at
labeled_batch rows=8 feature_width=3
== xy payload state ==
has_label=1 has_label_feature=0
```

### Example 2

The methods Example 1 does not exercise: `weights` by column name and by
`Series`, `groups`, `time(TimeOptions)`, and the two reduction methods. Every
"Effect" line quoted next to a setter above comes from this program's output.

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
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.6, 1.0, 1.3, 1.6}, {4}, f64))},
      {"sample_weight", Thread(pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 1, 1}, {4}, f64))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z", "2026-01-04T00:00:00Z"}))},
  });

  workflow::Payload by_column = workflow::payload::make(raw)
                                    .features({"income", "balance"})
                                    .label("default")
                                    .weights("sample_weight");
  std::cout << "weights(\"sample_weight\"): has_weights=" << std::boolalpha << by_column.has_weights()
            << " has_weight_feature=" << by_column.has_weight_feature()
            << " has_external_weights=" << by_column.has_external_weights() << "\n";

  const pluss::Series w = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64);
  workflow::Payload by_series = workflow::payload::make(raw)
                                    .features({"income", "balance"})
                                    .label("default")
                                    .weights(w);
  std::cout << "weights(Series):    has_weights=" << by_series.has_weights()
            << " has_weight_feature=" << by_series.has_weight_feature()
            << " has_external_weights=" << by_series.has_external_weights() << "\n";

  const pluss::Series groups = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  workflow::Payload grouped = workflow::payload::make(raw)
                                  .features({"income", "balance"})
                                  .label("default")
                                  .groups(groups);
  std::cout << "groups(Series):     has_groups=" << grouped.has_groups() << "\n";

  workflow::TimeOptions time_options{};
  time_options.index = "observed_at";
  time_options.horizon = workflow::steps(1);
  workflow::Payload with_time_options = workflow::payload::make(raw)
                                            .features({"income", "balance"})
                                            .label("default")
                                            .time(time_options);
  std::cout << "time(TimeOptions):  time_feature=" << with_time_options.time_feature()
            << " time_options().has_value()=" << with_time_options.time_options().has_value()
            << " horizon=" << with_time_options.time_options()->horizon << "\n";

  const lymph::FeatureBatch fb = by_column.feature_batch();
  const lymph::LabeledBatch lb = by_column.labeled_batch();
  std::cout << "feature_batch(): rows=" << fb.sample_count() << " cols=" << fb.feature_count() << "\n";
  std::cout << "labeled_batch(): rows=" << lb.features.sample_count()
            << " has_weights=" << lb.has_weights() << "\n";

  return 0;
}
```

Output

```text
weights("sample_weight"): has_weights=true has_weight_feature=true has_external_weights=false
weights(Series):    has_weights=true has_weight_feature=false has_external_weights=true
groups(Series):     has_groups=true
time(TimeOptions):  time_feature=observed_at time_options().has_value()=true horizon=1
feature_batch(): rows=4 cols=2
labeled_batch(): rows=4 has_weights=true
```

## Methods

None as separate pages — `Payload` is a self-contained value type. Every
member function is documented above under State / Configuration / Reduction.

## Changelog

- **0.1** — `Payload` = frame/features/labeled union + label/weights/groups/
  time role attachment + `feature_batch()`/`labeled_batch()` reduction.

## See Also

- [`Workflow`](Workflow.md) — every data verb accepts a `Payload`.
- [`Policy`](Policy.md) — applied to a `Payload`'s frame exactly as it would a
  bare raw-frame argument.
- [`TimeOptions`](TimeOptions.md) — the richer form of `.time(...)`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
