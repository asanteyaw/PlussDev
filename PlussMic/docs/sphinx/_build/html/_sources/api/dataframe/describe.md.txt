# DataFrame::describe

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::describe(bool render = false) -> DataFrameView
```

Compute descriptive statistics for the numeric features in the current frame.

`describe()` is the compact whole-frame summary route when you want counts, central tendency, spread, and quartiles instead of the original rows.

For ordinary numeric grading, sales, or sensor tables, this is the fastest way to inspect distribution shape before you start filtering, grouping, modeling, or plotting.

## Parameters

**render**

Display toggle for the returned view.

- `false`: keep explicit control over when and how you print or render the result.
- `true`: allow the returned `DataFrameView` to auto-render in temporary display contexts.

## Returns

`DataFrameView`
    Summary view whose rows are statistics such as `count`, `mean`, and percentiles, and whose columns are the participating numeric features.

## See Also

- <a href="mean.html"><code>DataFrame::mean</code></a>, <a href="std.html"><code>DataFrame::std</code></a>: one specific statistic instead of the full descriptive table.
- <a href="../thread/describe.html"><code>Thread::describe</code></a>: summarize one extracted feature directly.

## Notes

`describe()` summarizes numeric series features only. Note, timeline, and other non-numeric payloads are skipped instead of coerced.

Missing numeric values are ignored while the summary statistics are computed.

The summary rows are returned in a fixed statistical order: `count`, `mean`, `std`, `min`, `25%`, `50%`, `75%`, `max`.

`render` changes display behavior only. It does not change which statistics are computed or how missing values are handled.

## Examples

Example 1. Summarize every numeric feature in the frame.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
    {"city", pluss::make_note({"A", "B", "C", "D"})},
});

auto out = corr_base.describe(false);
```

### Output

```text
=============================================
      | x            y            z
---------------------------------------------
count | 4            4            4
mean  | 2.5          5            2.5
std   | 1.290994449  2.581988897  1.290994449
min   | 1            2            1
25%   | 1.75         3.5          1.75
50%   | 2.5          5            2.5
75%   | 3.25         6.5          3.25
max   | 4            8            4
---------------------------------------------
[8 cs x 3 feat]
```

Example 2. Use `describe()` to confirm that non-numeric features are ignored rather than coerced.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
    {"city", pluss::make_note({"A", "B", "C", "D"})},
});

auto out = corr_base.describe(false);
```

### Output

```text
=============================================
      | x            y            z
---------------------------------------------
count | 4            4            4
mean  | 2.5          5            2.5
std   | 1.290994449  2.581988897  1.290994449
min   | 1            2            1
25%   | 1.75         3.5          1.75
50%   | 2.5          5            2.5
75%   | 3.25         6.5          3.25
max   | 4            8            4
---------------------------------------------
[8 cs x 3 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="div.html">
    <span class="label">Next</span>
    <span class="title">div</span>
  </a>
</div>
