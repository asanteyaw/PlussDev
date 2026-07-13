# DataFrame::mean

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::mean(Axis axis = Axis::Case, bool skipna = true) -> Thread
```

## What It Does

Computes per-feature means for the selected numeric features.

## Parameters

**axis**
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

**skipna**
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Compute `mean` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mean(false);
```

### Output

```text
feature  value
id       2.0
sales    103.333333333
```

## Practical Behavior

- `mean(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="mean_by.html">
    <span class="label">Next</span>
    <span class="title">mean_by</span>
  </a>
</div>
