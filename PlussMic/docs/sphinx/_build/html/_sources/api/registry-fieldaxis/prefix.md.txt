# RegistryFieldAxisAccessor::prefix

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
RegistryFieldAxisAccessor::prefix(std::string_view token) -> Registry
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**token**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto fieldaxis = rg.fieldaxis();

auto out = fieldaxis.prefix("units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nulls.html">
    <span class="label">Previous</span>
    <span class="title">nulls</span>
  </a>
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
