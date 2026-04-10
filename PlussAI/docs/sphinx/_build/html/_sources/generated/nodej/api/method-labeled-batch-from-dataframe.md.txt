# `labeled_batch_from_dataframe`

## API Signature

- `labeled_batch_from_dataframe(frame: const pluss::table::DataFrame&, options: const DataFrameBatchOptions&) -> StatusOr<LabeledBatch>`

## Notes

Builds a supervised `LabeledBatch` from a DataFrame. Use `options` to pick feature columns, label column, weight column, and categorical handling policy.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `options` (`const DataFrameBatchOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `StatusOr<LabeledBatch>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `DataFrameBatchOptions` (`struct`):
  - `std::vector<std::string> feature_names{}`
    Explicit list of feature columns to read from the input frame.
  - `std::optional<std::string> label_feature{std::nullopt}`
    Column used as the supervised target (`y`).
  - `std::optional<std::string> sample_weight_feature{std::nullopt}`
    Optional column used as per-row training weight.
  - `ObservationLayout layout{ObservationLayout::Rows}`
    Declares whether observations are represented by rows or columns.
  - `DataThreadPolicy thread_policy{DataThreadPolicy::CastTemporalToSeries}`
    Controls how numeric/temporal frame columns are adapted into `Series`.
  - `DataNotePolicy note_policy{DataNotePolicy::Reject}`
    Controls how note/categorical columns are handled during adaptation.
  - `std::uint64_t note_hash_seed{1469598103934665603ULL}`
    Random seed for reproducible behavior.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = labeled_batch_from_dataframe(/* frame */, /* options */);
if (!res.ok()) {
  out.error("labeled_batch_from_dataframe failed", res.status().message);
} else {
  out.info("labeled_batch_from_dataframe ok");
}
```

**Expected output**

```text
labeled_batch_from_dataframe: ok
status.ok() == true
result.value() is available
```

### Example 2 (Demo Pattern)

```cpp
  DataFrameBatchOptions options{};
  options.feature_names = {"x0", "x1", "category", "ts", "target", "weight"};
  options.label_feature = "target";
  options.sample_weight_feature = "weight";
  options.note_policy = DataNotePolicy::Hash;
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;

  auto batch_or = labeled_batch_from_dataframe(frame, options);
  if (!require_ok(batch_or, "labeled_batch_from_dataframe")) return 1;

  const LabeledBatch& batch = batch_or.value();
  std::cout << "adapted batch: samples=" << batch.features.sample_count()
            << " features=" << batch.features.feature_count()
            << " weighted=" << (batch.sample_weights.defined() ? "yes" : "no") << "\n";

  ModelSpec spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
```

**Expected output**

```text
NodeJ DataFrame bridge demo
adapted batch: samples=6 features=4 weighted=yes
linear metrics [mse, r2]: 1.5165e-17 1
NodeJ DataFrame bridge demo completed.
```
