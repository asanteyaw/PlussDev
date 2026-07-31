# RegistryFieldAxisAccessor::series

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
RegistryFieldAxisAccessor::series() -> Registry
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto fieldaxis = rg.fieldaxis();

auto out = fieldaxis.series();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prefix.html">
    <span class="label">Previous</span>
    <span class="title">prefix</span>
  </a>
  <a class="next-link" href="suffix.html">
    <span class="label">Next</span>
    <span class="title">suffix</span>
  </a>
</div>
