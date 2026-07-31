# Registry::to_json

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::to_json() -> std::string
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

auto out = rg.to_json();
```

### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take_values_.html">
    <span class="label">Previous</span>
    <span class="title">take_values_</span>
  </a>
  <a class="next-link" href="to_kindle.html">
    <span class="label">Next</span>
    <span class="title">to_kindle</span>
  </a>
</div>
