# DataFrame::var

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::var(Axis axis = Axis::Case, bool skipnull = true) -> Thread

DataFrame::var(Axis axis, const StatisticOptions& options) -> Thread
```

## What It Does

Computes per-feature variances for the selected numeric features.

## Parameters

**axis**
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

**skipnull**
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Compute `var` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.var(false);
```

### Output

```text
feature  value
id       1.0
sales    233.333333333
```
Example 2. Compute `var` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.var(false);
```

### Output

```text
feature  value
id       1.0
sales    233.333333333
```

## Practical Behavior

- `var(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="update.html">
    <span class="label">Previous</span>
    <span class="title">update</span>
  </a>
  <a class="next-link" href="view.html">
    <span class="label">Next</span>
    <span class="title">view</span>
  </a>
</div>
