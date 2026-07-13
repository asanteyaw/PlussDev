# DataFrame::flow

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::flow(const std::function<DataFrame(const DataFrame&)>& fn) -> DataFrame
```

## What It Does

Computes a cumulative directional summary across numeric features for each row.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

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

auto out = df.flow();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floordiv.html">
    <span class="label">Previous</span>
    <span class="title">floordiv</span>
  </a>
  <a class="next-link" href="fold.html">
    <span class="label">Next</span>
    <span class="title">fold</span>
  </a>
</div>
