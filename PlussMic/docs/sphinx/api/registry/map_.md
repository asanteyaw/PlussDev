<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 966880a19458 -->

# Registry::map_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry& Registry::map_(const std::function<Slot(const Slot&)>& fn)
```

## What It Does

The in-place counterpart to [`map()`](map.md) — runs `fn` over every field's
`Slot` and rewrites this registry directly.

```{important} Whole-registry transform
`fn` runs on every field regardless of value kind, so a callback written only for
one kind can throw on a field of a different kind. To scope a `Slot -> Slot`
transform to one field, fetch it via [`fields(key)`](fields.md), call `fn` on it,
and reinsert with [`set_()`](set_.md): `rg.set_("population", fn(rg.fields("population")))`.
```

## Parameters

**fn** (`const std::function<Slot(const Slot&)>&`)
    A callback taking each field's `Slot` in turn and returning its replacement.
    Must return a defined `Slot` for every field.

## Returns

`Registry&`
    A reference to this registry, now holding every field transformed by `fn`.

## Examples

Example 1. Add 5 to every value in a registry, in place.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
});

rg.map_([](const pluss::scribble::Slot& s) {
  return s.transform([](const pluss::scribble::Slot::Value& v) -> pluss::scribble::Slot::Value {
    return pluss::make_series(std::get<pluss::Series>(v).host<std::int64_t>()[0] + 5);
  });
});

auto ids = rg.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).host<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[6, 7]
```

## Notes

- Performs a full-registry reassignment under the hood, not a field-by-field
  mutation of the existing slots.
- Mutates the registry it's called on and returns `*this` by reference.

## See Also

- [`Registry::map`](map.md): the non-mutating counterpart this method wraps.
- [`Registry::fields`](fields.md) / [`Registry::set_`](set_.md): fetch-transform-reinsert
  for scoping a `Slot -> Slot` transform to one named field in place.
- [`Registry::morph_`](morph_.md): a value-level (not whole-`Slot`) in-place
  transform, scoped to one field.
- [`Catalog::map`](../catalog/map.md): the closest equivalent on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="maxstate.html">
    <span class="label">Next</span>
    <span class="title">maxstate</span>
  </a>
</div>
