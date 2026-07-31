# Registry::min

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::min(std::string_view field) -> Slot
```

## What It Does

Use this when you want a summary view instead of carrying the full input forward unchanged.

## Parameters

**field**
    Name of the target stream in the catalog.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Compute `min` and inspect the resulting summary value or summary object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.min();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
