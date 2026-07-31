# RegistryFieldAccessor::nt

[Back to RegistryFieldAccessor API Reference](../../registry_field_accessor_api_reference)

## Signature

```cpp
RegistryFieldAccessor::nt() -> RegistryNoteAccessor
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`RegistryNoteAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto fields = rg.fields({"ids"});

auto out = fields.nt();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
