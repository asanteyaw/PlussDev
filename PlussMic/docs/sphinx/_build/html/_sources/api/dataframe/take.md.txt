# DataFrame::take

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::take(pluss::RangeView<std::int64_t> indices) -> DataFrame
```

## What It Does

Returns the rows at the requested integer positions.

## Parameters

**indices**
    Integer positions used to select, reorder, or address entries.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Pass several integer positions together.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::vector<std::int64_t> picks{0, 2};
auto out = df.take(picks);
```

### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="tally.html">
    <span class="label">Next</span>
    <span class="title">tally</span>
  </a>
</div>
