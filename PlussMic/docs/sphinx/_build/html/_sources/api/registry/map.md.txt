<!-- pluss-hand-authored -->

# Registry::map

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::map(const std::function<Slot(const Slot&)>& fn) const
```

## What It Does

A pure alias for [`apply_all()`](apply_all.md) — runs `fn` over every field's
`Slot` and returns a new registry with all of them replaced. `map()` exists for
readability at call sites that read naturally as "map this function over every
field"; `apply_all()` exists for call sites where "apply to every slot,
unconditionally" (as opposed to `apply()`'s scoped form) is the clearer framing.
There is no behavioral difference.

```{important} Same mixed-kind hazard as apply_all()
Since `map()` is a direct alias, it inherits `apply_all()`'s caveat: `fn` runs on
every field regardless of value kind, so a callback written only for one kind can
throw on a field of a different kind. See [`apply_all()`](apply_all.md)'s important
callout.
```

## Parameters

**fn** (`const std::function<Slot(const Slot&)>&`)
    A callback taking each field's `Slot` in turn and returning its replacement.
    Must return a defined `Slot` for every field.

## Returns

`Registry`
    A new registry with every field replaced by `fn`'s result — identical to what
    `apply_all(fn)` would return.

## Examples

Example 1. `map()` and `apply_all()` produce identical results for the same `fn`.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

auto out = rg.map([](const pluss::bundle::Slot& s) { return s; });
std::cout << out.nfields();
```

### Output

```text
1
```

## Notes

- Identical to `apply_all()` in every respect — pick whichever name reads better
  at the call site.
- Never mutates the receiver — returns a new `Registry`; use [`map_()`](map_.md)
  for the in-place form.

## See Also

- [`Registry::apply_all`](apply_all.md): the method this is a pure alias for.
- [`Registry::map_`](map_.md): the in-place counterpart.
- [`Registry::apply`](apply.md): the scoped counterpart, for transforming only
  named fields.
- [`Catalog::apply_all`](../catalog/apply_all.md): the closest equivalent on the
  kindle model — `Catalog` has no `map`/`map_` alias, only `apply_all`.

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
