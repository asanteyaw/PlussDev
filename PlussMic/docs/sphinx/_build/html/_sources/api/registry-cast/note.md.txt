# RegistryCastAccessor::note

[Back to RegistryCastAccessor API Reference](../../registry_cast_accessor_api_reference)

## Signature

```cpp
RegistryCastAccessor::note() -> Registry
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
auto cast = rg.fields({"ids"}).cast();

auto out = cast.note();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
