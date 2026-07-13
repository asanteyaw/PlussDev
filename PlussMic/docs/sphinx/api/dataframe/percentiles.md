# DataFrame::percentiles

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::percentiles(pluss::RangeView<double> points = {0.25, 0.5, 0.75}, std::int64_t min_count = 1) -> DataFrame
```

## What It Does

Computes requested numeric distribution points for every numeric feature.

## Parameters

**points**
    Requested evaluation-point count for density-style plots. Higher values make smoother curves at higher compute cost.

**min_count**
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Compute several requested percentile points for each numeric feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::vector<double> points{0.25, 0.5, 0.75};
auto out = df.percentiles(points, 1);
```

### Output

```text
p      id   sales
0.25   1.5  95.0
0.50   2.0  100.0
0.75   2.5  110.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="observe.html">
    <span class="label">Previous</span>
    <span class="title">observe</span>
  </a>
  <a class="next-link" href="pivot.html">
    <span class="label">Next</span>
    <span class="title">pivot</span>
  </a>
</div>
