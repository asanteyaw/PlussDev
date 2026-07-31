# Registry::as_kindle

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::as_kindle() -> pluss::kindle::Catalog
```

## What It Does

Converts or reinterprets the current payload as another public Pluss type so downstream operations route through the right text, numeric, or temporal surface.

## Parameters

`None`

## Returns

`pluss::kindle::Catalog`
    Value produced by this call.

## Examples

Example 1. Convert the current object into the `kindle` representation before continuing with another type-specific workflow.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.as_kindle();
```

### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

## Bridge Behavior

- `as_kindle()` converts the registry into the Kindle/Catalog model.
- Use this when the next operation is stream-oriented rather than bundle-oriented.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply_all.html">
    <span class="label">Previous</span>
    <span class="title">apply_all</span>
  </a>
  <a class="next-link" href="begin.html">
    <span class="label">Next</span>
    <span class="title">begin</span>
  </a>
</div>
