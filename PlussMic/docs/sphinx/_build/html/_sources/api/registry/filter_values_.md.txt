# Registry::filter_values_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::filter_values_(std::string_view key, const std::function<bool(const Slot::Value&)>& predicate) -> Registry&
```

## What It Does

Filters the values in one named slot in place.

## Parameters

**key**
    Name of the target slot or stream.

**predicate**
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

rg.filter_values_("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
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
  <a class="prev-link" href="filter_values.html">
    <span class="label">Previous</span>
    <span class="title">filter_values</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
