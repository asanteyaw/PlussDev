# Axel::as_kindle

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::as_kindle() -> pluss::kindle::Archive
```

## What It Does

Converts or reinterprets the current payload as another public Pluss type so downstream operations route through the right text, numeric, or temporal surface.

## Parameters

`None`

## Returns

`pluss::kindle::Archive`
    Value produced by this call.

## Examples

Example 1. Convert the current object into the `kindle` representation before continuing with another type-specific workflow.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.as_kindle();
```

### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

## Bridge Behavior

- `as_kindle()` converts the axel into the Archive model.
- Use this when the next workflow is archive/catalog based rather than bundle based.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply_slot.html">
    <span class="label">Previous</span>
    <span class="title">apply_slot</span>
  </a>
  <a class="next-link" href="concat_depth.html">
    <span class="label">Next</span>
    <span class="title">concat_depth</span>
  </a>
</div>
