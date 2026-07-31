# RegistryFieldAccessor::tl

[Back to RegistryFieldAccessor API Reference](../../registry_field_accessor_api_reference)

## Signature

```cpp
RegistryFieldAccessor::tl() -> RegistryTimelineAccessor
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`RegistryTimelineAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto fields = rg.fields({"ids"});

auto out = fields.tl();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nt.html">
    <span class="label">Previous</span>
    <span class="title">nt</span>
  </a>
</div>
