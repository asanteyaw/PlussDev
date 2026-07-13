# DataFrame::mask

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::mask(const Thread& bool_mask, pluss::RangeView<std::string_view> features = {}) -> DataFrame
```

## What It Does

Keeps original values where a boolean mask is false and substitutes new values where the mask is true.

## Parameters

**bool_mask**
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mask();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="match.html">
    <span class="label">Next</span>
    <span class="title">match</span>
  </a>
</div>
