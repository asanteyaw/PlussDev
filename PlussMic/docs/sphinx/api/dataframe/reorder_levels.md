# DataFrame::reorder_levels

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::reorder_levels(pluss::RangeView<std::string_view> names) -> DataFrame

DataFrame::reorder_levels(std::initializer_list<std::string_view> names) -> DataFrame
```

## What It Does

Reorders the current case-axis levels and recomputes the composite row labels from that new level order.

## Parameters

**names**
    Collection of feature, slot, or stream names supplied in positional order.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.reorder_levels();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.reorder_levels();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reorder.html">
    <span class="label">Previous</span>
    <span class="title">reorder</span>
  </a>
  <a class="next-link" href="repeated_cases.html">
    <span class="label">Next</span>
    <span class="title">repeated_cases</span>
  </a>
</div>
