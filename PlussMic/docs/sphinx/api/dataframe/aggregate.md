# DataFrame::aggregate

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::aggregate(pluss::RangeView<SummarySpec> specs, bool render = false) -> DataFrameView
```

Build a one-row summary view from one or more aggregation specifications.

Use `DataFrame::aggregate(...)` when you want compact summary output rather than grouped rows. Each `SummarySpec` tells PlussMic which source feature to read, which summary operation to compute, and what output feature name to place in the one-row result.

## Parameters

**specs**

Collection of `SummarySpec` instructions.

Each spec supplies three pieces of information:

- `output`: the output feature name. Leave it empty when you want PlussMic to auto-name the metric.
- `feature`: the source feature to summarize.
- `op`: the summary operation to compute.

Use one spec for one metric, or several specs when you want a compact dashboard row from one call.

**render**

Display policy for the returned `DataFrameView`.

- `false`: return the view without enabling auto-render behavior.
- `true`: allow the summary view to auto-render in temporary display contexts.

Most library-style code should keep this `false` and render explicitly.

## Returns

`DataFrameView`
    A one-row summary view.

    - The result contains one case labeled `summary`.
    - Each requested metric becomes one output feature.
    - The `render` flag controls whether the returned view may auto-render in temporary display contexts.

## See Also

- <a href="describe.html"><code>DataFrame::describe</code></a>: produce a broader canned descriptive summary.
- <a href="#group"><code>DataFrame::group</code></a>: aggregate within key-defined groups instead of collapsing the whole frame to one row.
- <a href="#summarize"><code>DataFrame::summarize</code></a>: return the summary result as a raw frame-oriented object when that surface is a better fit.

## Notes

`aggregate(...)` returns a `DataFrameView`, not a raw `DataFrame`. This is useful when you want summary output that can display cleanly without manually calling a renderer first.

If you leave the `output` field empty in a `SummarySpec`, PlussMic auto-generates a name from the source feature and operation, such as `assignment_mean`.

Numeric summary operations expect numeric-compatible input. Count-like operations can summarize a broader set of feature kinds.

## Examples

Example 1. Compute one summary metric with an explicit output name.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");

auto out = grades.aggregate({
    {"mean_final", "final", DataFrame::SummaryOp::Mean},
}, false);
```

### Output

```text
====================
        | mean_final
--------------------
summary | 24.5
--------------------
[1 cs x 1 feat]
```

Example 2. Compute several summary metrics in one pass.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");

auto out = grades.aggregate({
    {"n_students", "final", DataFrame::SummaryOp::Count},
    {"max_final", "final", DataFrame::SummaryOp::Max},
    {"min_final", "final", DataFrame::SummaryOp::Min},
}, false);
```

### Output

```text
==========================================
        | n_students  max_final  min_final
------------------------------------------
summary | 3           28         21.5
------------------------------------------
[1 cs x 3 feat]
```

Example 3. Let PlussMic auto-name the output summary features.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");

auto out = grades.aggregate({
    {"", "assignment", DataFrame::SummaryOp::Mean},
    {"", "assignment", DataFrame::SummaryOp::Std},
}, false);
```

### Output

```text
=========================================
        | assignment_mean  assignment_std
-----------------------------------------
summary | 8.333333333      1.040833
-----------------------------------------
[1 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_suffix.html">
    <span class="label">Previous</span>
    <span class="title">add_suffix</span>
  </a>
  <a class="next-link" href="align_cases.html">
    <span class="label">Next</span>
    <span class="title">align_cases</span>
  </a>
</div>
