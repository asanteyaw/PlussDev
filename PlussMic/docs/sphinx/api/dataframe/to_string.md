# DataFrame::to_string

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::to_string(const display::DisplayOptions& options = {}) -> std::string
```

## What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`std::string`
    Rendered string result.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.to_string();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tl.html">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="to_utc.html">
    <span class="label">Next</span>
    <span class="title">to_utc</span>
  </a>
</div>
