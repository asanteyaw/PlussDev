# `lymph::Payload`

## Summary

`lymph::Payload` is the typed data-and-metadata carrier used around workflow
execution.

Use it when the modeling boundary carries more than just `X` and `y`.

`Payload` exists for the real project case where execution depends on side-channel
roles such as:

- labels
- weights
- groups
- time
- feature-name intent
- table-to-batch conversion policy

## Why It Exists

Serious learning systems often need richer context than a plain feature matrix:

- supervised work needs a label role
- grouped validation needs entity keys
- time-aware validation needs a time role
- table-first modeling needs explicit feature selection

`Payload` makes that contract explicit without forcing callers into a loose set of
parallel arguments that drift out of sync.

## Signature

```cpp
lymph::Payload lymph::payload::make(
    pluss::table::DataFrame frame,
    lymph::DataFrameBatchOptions options = {});
lymph::Payload& lymph::Payload::features(std::initializer_list<std::string> names);
lymph::Payload& lymph::Payload::label(std::string_view feature_name);
lymph::Payload& lymph::Payload::time(std::string_view feature_name);
lymph::LabeledBatch lymph::Payload::labeled_batch() const;
```

## Practical Example

This page uses the exact verifier program
`lymph_workflow_payload_docverify_demo.cpp`.

### Example 1

This example starts from a raw `DataFrame`, attaches feature, label, and time
roles, materializes the labeled batch, and prints the resulting payload state.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"customer_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106, 107, 108}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z",
          "2026-01-08T00:00:00Z",
      }))},
  });

  lymph::Payload payload = lymph::payload::make(raw)
                               .features({"income", "balance", "late", "region"})
                               .label("default")
                               .time("observed_at")
                               .note(lymph::DataNotePolicy::Factorize);

  const lymph::LabeledBatch labeled = payload.labeled_batch();
  std::cout << "== payload ==\n";
  std::cout << "has_frame=" << payload.has_frame()
            << " has_label_feature=" << payload.has_label_feature()
            << " has_time=" << payload.has_time()
            << " time_feature=" << payload.time_feature() << "\n";
  std::cout << "batch_rows=" << labeled.features.sample_count()
            << " feature_width=" << labeled.features.feature_count() << "\n";

  return 0;
}
```

Output

```text
== payload ==
has_frame=1 has_label_feature=1 has_time=1 time_feature=observed_at
batch_rows=8 feature_width=4
```

This is the actual value of `Payload`:

- the underlying frame is still present
- supervised role declaration is explicit
- time intent is attached by name instead of guessed
- the object can materialize into a labeled batch with the expected width

## Related Pages

- [Workflow](Workflow.md)
- [Model](Model.md)
- [Policy](Policy.md)

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

```
<!-- apiref:toctree:end -->
