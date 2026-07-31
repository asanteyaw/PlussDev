# RegistryFieldAxisAccessor::empty

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
RegistryFieldAxisAccessor::empty() -> Registry
```

## What It Does

Reports whether the current object has no rows, entries, slots, or depth items.

## Parameters

`None`

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Inspect the `empty` property of the current object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto fieldaxis = rg.fieldaxis();

auto out = fieldaxis.empty();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="kind.html">
    <span class="label">Next</span>
    <span class="title">kind</span>
  </a>
</div>
