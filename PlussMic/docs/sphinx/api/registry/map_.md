<!-- pluss-hand-authored -->

# Registry::map_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry& Registry::map_(const std::function<Slot(const Slot&)>& fn)
```

## What It Does

The in-place counterpart to [`map()`](map.md)/[`apply_all()`](apply_all.md) — runs
`fn` over every field's `Slot` and rewrites this registry directly. Implemented as
`*this = apply_all(fn)`.

```{important} Same mixed-kind hazard as apply_all()
`fn` runs on every field regardless of value kind — see
[`apply_all()`](apply_all.md)'s important callout.
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
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});

rg.map_([](const pluss::bundle::Slot& s) {
  return s.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
    return pluss::make_series(std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] + 5);
  });
});

auto ids = rg.select("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[6, 7]
```

## Notes

- Implemented as `*this = apply_all(fn)` — full-registry reassignment under the
  hood, not a field-by-field in-place mutation.
- Mutates the registry it's called on and returns `*this` by reference.

## See Also

- [`Registry::map`](map.md) / [`Registry::apply_all`](apply_all.md): the
  non-mutating counterparts this method wraps.
- [`Registry::apply_`](apply_.md): the in-place, scoped counterpart, for
  transforming only named fields.
- [`Catalog::apply_all`](../catalog/apply_all.md): the closest equivalent on the
  kindle model — `Catalog` has no `map`/`map_` alias, only `apply_all`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
