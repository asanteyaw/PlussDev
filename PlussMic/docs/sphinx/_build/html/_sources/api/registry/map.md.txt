<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9b5f8eccadfc -->

# Registry::map

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::map(const std::function<Slot(const Slot&)>& fn) const
```

## What It Does

Runs `fn` over every field's `Slot` and returns a new registry with all of them
replaced.

```{important} Whole-registry transform
`fn` runs on every field regardless of value kind, so a callback written only for
one kind can throw on a field of a different kind. `Registry` has no scoped,
apply-to-named-fields sibling method anymore — to transform just one field, fetch
it via [`fields(key)`](fields.md), call `fn` on it directly, and reinsert with
[`set()`](set.md): `rg.set("population", fn(rg.fields("population")))`.
```

## Parameters

**fn** (`const std::function<Slot(const Slot&)>&`)
    A callback taking each field's `Slot` in turn and returning its replacement.
    Must return a defined `Slot` for every field.

## Returns

`Registry`
    A new registry with every field replaced by `fn`'s result.

## Examples

Example 1. Apply one transform to every field.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

auto out = rg.map([](const pluss::scribble::Slot& s) { return s; });
std::cout << out.nfields();
```

### Output

```text
1
```

## Notes

- Never mutates the receiver — returns a new `Registry`; use [`map_()`](map_.md)
  for the in-place form.

## See Also

- [`Registry::map_`](map_.md): the in-place counterpart.
- [`Registry::fields`](fields.md) / [`Registry::set`](set.md): fetch-transform-reinsert
  for scoping a `Slot -> Slot` transform to one named field instead of every field.
- [`Registry::morph`](morph.md): a value-level (not whole-`Slot`) transform, scoped
  to one field.
- [`Catalog::map`](../catalog/map.md): the closest equivalent on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_txt.html">
    <span class="label">Previous</span>
    <span class="title">load_txt</span>
  </a>
  <a class="next-link" href="map_.html">
    <span class="label">Next</span>
    <span class="title">map_</span>
  </a>
</div>
