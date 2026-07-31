# Registry::take_values_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::take_values_(std::string_view key, std::span<const std::int64_t> indices) -> Registry&
```

## What It Does

Selects values from one named slot by integer position in place.

## Parameters

**key**
    Name of the target slot or stream.

**indices**
    Integer positions used to select, reorder, or address entries.

## Returns

`Registry&`
    Value produced by this call.

## Examples

Example 1. Pass several integer positions together.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
rg.take_values_("ids", picks);
```

### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take_values.html">
    <span class="label">Previous</span>
    <span class="title">take_values</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
