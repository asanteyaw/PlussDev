# DataFrame::max

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::max(Axis axis = Axis::Case, bool skipna = true) -> Thread
```

## What It Does

Computes per-feature maxima for the selected numeric features.

## Parameters

**axis**
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

**skipna**
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Compute `max` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.max(false);
```

### Output

```text
feature  value
id       3
sales    120.0
```

## Practical Behavior

- `max(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matrix.html">
    <span class="label">Previous</span>
    <span class="title">matrix</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
