# DataFrame::std

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::std(Axis axis = Axis::Case, bool skipnull = true) -> Thread

DataFrame::std(Axis axis, const StatisticOptions& options) -> Thread
```

## What It Does

Computes per-feature standard deviations for the selected numeric features.

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

Example 1. Compute `std` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.std(false);
```

### Output

```text
feature  value
id       1.0
sales    15.275252317
```
Example 2. Compute `std` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.std(false);
```

### Output

```text
feature  value
id       1.0
sales    15.275252317
```

## Practical Behavior

- `std(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stack.html">
    <span class="label">Previous</span>
    <span class="title">stack</span>
  </a>
  <a class="next-link" href="sub.html">
    <span class="label">Next</span>
    <span class="title">sub</span>
  </a>
</div>
