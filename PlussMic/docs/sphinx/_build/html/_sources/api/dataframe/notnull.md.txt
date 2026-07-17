# DataFrame::notnull

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::notnull(pluss::RangeView<std::string_view> features = {}) -> DataFrame
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto sales = pluss::table::Thread(pluss::make_series({100.0, std::numeric_limits<double>::quiet_NaN(), 90.0}));
auto df = pluss::table::DataFrame({
    {"sales", sales},
});

auto out = df.notnull({"sales"});
```

### Output

```text
sales
true
false
true
```

## Practical Behavior

- `notnull(...)` returns a boolean-style frame marking which cells are observed rather than missing.
- Use it to build masks before `filter(...)`, replacement policies, or diagnostic summaries.
- `notna(...)` and `notnull(...)` are equivalent.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="observe.html">
    <span class="label">Next</span>
    <span class="title">observe</span>
  </a>
</div>
