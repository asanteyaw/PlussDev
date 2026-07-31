# Registry::to_string

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::to_string() -> std::string
```

## What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

## Parameters

`None`

## Returns

`std::string`
    Rendered string result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_string();
```

### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_kindle.html">
    <span class="label">Previous</span>
    <span class="title">to_kindle</span>
  </a>
  <a class="next-link" href="transform_values.html">
    <span class="label">Next</span>
    <span class="title">transform_values</span>
  </a>
</div>
