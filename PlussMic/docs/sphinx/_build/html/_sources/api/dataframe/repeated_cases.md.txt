# DataFrame::repeated_cases

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::repeated_cases(pluss::RangeView<std::string_view> features = {}) -> pluss::Series
```

## What It Does

Returns the rows whose current case labels are duplicates of an earlier row.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

## Returns

`pluss::Series`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto dup = df.with_cases(pluss::table::Thread(pluss::make_note({"cs0", "cs0", "cs2"})));
auto out = dup.repeated_cases();
```

### Output

```text
[false, true, false]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reorder_levels.html">
    <span class="label">Previous</span>
    <span class="title">reorder_levels</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
