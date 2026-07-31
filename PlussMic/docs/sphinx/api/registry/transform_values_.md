# Registry::transform_values_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::transform_values_(std::string_view key, const std::function<Slot::Value(const Slot::Value&)>& fn) -> Registry&
```

## What It Does

Rewrites the values inside one named slot in place.

## Parameters

**key**
    Name of the target slot or stream.

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Registry&`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.transform_values_("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
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
  <a class="prev-link" href="transform_values.html">
    <span class="label">Previous</span>
    <span class="title">transform_values</span>
  </a>
  <a class="next-link" href="type_profiles.html">
    <span class="label">Next</span>
    <span class="title">type_profiles</span>
  </a>
</div>
