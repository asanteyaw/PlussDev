# DataFrame::sum

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::sum(Axis axis = Axis::Case, bool skipna = true) -> Thread
```

## What It Does

Computes per-feature sums for the selected numeric features.

## Parameters

**axis**
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

**skipna**
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Compute `sum` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sum(false);
```

### Output

```text
feature  value
id       6
sales    310.0
```

## Practical Behavior

- `sum(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sub.html">
    <span class="label">Previous</span>
    <span class="title">sub</span>
  </a>
  <a class="next-link" href="tabelize.html">
    <span class="label">Next</span>
    <span class="title">tabelize</span>
  </a>
</div>
