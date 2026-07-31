<!-- pluss-hand-authored -->

# Registry::apply_all

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::apply_all(const std::function<Slot(const Slot&)>& fn) const
```

## What It Does

Runs `fn` over **every** field's `Slot` and returns a new registry with all of them
replaced — the un-scoped counterpart to [`apply()`](apply.md), which only touches
the field(s) you name. [`map()`](map.md) is a pure alias for this method.

```{important} fn runs on every field, regardless of value kind
Because `apply_all()` doesn't let you scope by field name, `fn` must handle
whatever kind each field actually holds — a callback written only for `Series`
values will throw (or misbehave) the moment it hits a `Note`-kind field. If your
registry mixes field kinds and you only want to transform some of them, use
[`apply()`](apply.md) with an explicit key list instead.
```

## Parameters

**fn** (`const std::function<Slot(const Slot&)>&`)
    A callback taking each field's `Slot` in turn and returning its replacement.
    Must return a defined `Slot` for every field.

## Returns

`Registry`
    A new registry with every field replaced by `fn`'s result.

## Examples

Example 1. Multiply every field's values by 100 — safe here because every field
happens to be `Series`-kind.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"other_id", pluss::bundle::Slot({pluss::make_series(10), pluss::make_series(20)})},
});

auto out = rg.apply_all([](const pluss::bundle::Slot& s) {
  return s.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
    return pluss::make_series(std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] * 100);
  });
});

auto id_out = out.select("id");
auto other_out = out.select("other_id");
std::cout << std::get<pluss::Series>(id_out.at(0)).to_vec<std::int64_t>()[0] << " ";
std::cout << std::get<pluss::Series>(other_out.at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
100 1000
```

## Notes

- Applies to every field unconditionally — see the important callout above for the
  mixed-kind hazard.
- `fn` must return a defined `Slot` for every field, same requirement as
  [`apply()`](apply.md).
- Never mutates the receiver — returns a new `Registry`; use [`map_()`](map_.md)
  for the in-place form.

## See Also

- [`Registry::apply`](apply.md): the scoped counterpart, for transforming only
  named fields.
- [`Registry::map`](map.md): a pure alias for this method.
- [`Registry::map_`](map_.md): the in-place counterpart.
- [`Catalog::apply_all`](../catalog/apply_all.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply_.html">
    <span class="label">Previous</span>
    <span class="title">apply_</span>
  </a>
  <a class="next-link" href="as_kindle.html">
    <span class="label">Next</span>
    <span class="title">as_kindle</span>
  </a>
</div>
