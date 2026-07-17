# DataFrame::freq

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::freq(pluss::RangeView<std::string_view> features = {}, std::int64_t top_n = 0, bool dropnull = true) -> Thread
```

## What It Does

Counts the most frequent values or value-combinations across the selected features.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**top_n**
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

**dropnull**
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.freq();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fold.html">
    <span class="label">Previous</span>
    <span class="title">fold</span>
  </a>
  <a class="next-link" href="ge.html">
    <span class="label">Next</span>
    <span class="title">ge</span>
  </a>
</div>
