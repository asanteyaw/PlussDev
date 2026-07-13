# DataFrame::select

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::select(pluss::RangeView<std::string_view> names) -> DataFrame
```

## What It Does

Projects a selected feature subset into a new frame while preserving row order and case alignment.

## Parameters

**names**
    Collection of feature, slot, or stream names supplied in positional order.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Project only the named columns into a new frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.select({"id", "sales"});
```

### Output

```text
id  sales
1   100.0
2   120.0
3    90.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="se.html">
    <span class="label">Previous</span>
    <span class="title">se</span>
  </a>
  <a class="next-link" href="select_like.html">
    <span class="label">Next</span>
    <span class="title">select_like</span>
  </a>
</div>
