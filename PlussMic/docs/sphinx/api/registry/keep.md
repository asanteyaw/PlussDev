<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: de46b31745d7 -->

# Registry::keep

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::keep(std::string_view key, const std::function<bool(const Slot::Value&)>& predicate) const
```

## What It Does

Returns a new registry with one named field's `Slot` filtered down to only the
values for which `predicate` returns `true` — every other field is left
untouched. Built on [`Slot::filter()`](../slot/filter.md).

## Parameters

**key** (`std::string_view`)
    The field to filter. Throws if it doesn't exist.

**predicate** (`const std::function<bool(const Slot::Value&)>&`)
    A callback taking each of the field's values in turn; values for which it
    returns `false` are dropped.

## Returns

`Registry`
    A new registry with the named field's `Slot` filtered; every other field
    unchanged.

## Examples

Example 1. Keep only the odd values in `id`.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.keep("id", [](const pluss::scribble::Slot::Value& v) {
  return std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] % 2 == 1;
});

auto ids = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "] nfields=" << out.nfields();
```

### Output

```text
[1, 3] nfields=2
```

Example 2. Filtering an unknown field throws.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1)})},
});

try {
  auto out = rg.keep("zzz", [](const pluss::scribble::Slot::Value&) { return true; });
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::keep: unknown key 'zzz'
```

## Notes

- Scoped to exactly one field — the other fields' sizes are unaffected, so
  filtering can make the result ragged relative to those other fields.
- Throws `"Registry::keep: unknown key '<name>'"` for a missing field, and
  `"Registry::keep: predicate is empty"` for a null `predicate`.
- Never mutates the receiver — use [`keep_()`](keep_.md) for the in-place form.

## See Also

- [`Registry::keep_`](keep_.md): the in-place counterpart.
- [`Registry::morph`](morph.md): transform a field's values in place, rather
  than filtering them out.
- [`Registry::isnull`](isnull.md) / [`Registry::dropnull`](dropnull.md): the
  null-specific filtering shorthands, applied across every field.
- [`Catalog::filter`](../catalog/filter.md): the closest equivalent on the
  kindle model, filtering rows rather than one field's values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="keep_.html">
    <span class="label">Next</span>
    <span class="title">keep_</span>
  </a>
</div>
