<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f1e4525ca363 -->

# Registry::morph

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::morph(std::string_view key, const std::function<Slot::Value(const Slot::Value&)>& fn) const
```

## What It Does

Returns a new registry with one named field's values each individually
transformed by `fn` — every other field is left untouched. Built on
[`Slot::transform()`](../slot/transform.md). Unlike [`map()`](map.md), which
replaces a whole field's `Slot` (`Slot -> Slot`), `morph()` works one *value* at
a time (`Slot::Value Slot::Value`), preserving the field's length.

## Parameters

**key** (`std::string_view`)
    The field to transform. Throws if it doesn't exist.

**fn** (`const std::function<Slot::Value(const Slot::Value&)>&`)
    A callback taking each of the field's values in turn and returning its
    replacement.

## Returns

`Registry`
    A new registry with the named field's values transformed; every other field
    unchanged.

## Examples

Example 1. Multiply every value in `id` by 10.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.morph("id", [](const pluss::scribble::Slot::Value& v) -> pluss::scribble::Slot::Value {
  return pluss::make_series(std::get<pluss::Series>(v).host<std::int64_t>()[0] * 10);
});

auto ids = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).host<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[10, 20, 30]
```

## Notes

- One-value-at-a-time transform, preserving the field's length — contrast with
  [`map()`](map.md), which replaces a whole field's `Slot`.
- Throws `"Registry::morph: unknown key '<name>'"` for a missing field, and
  `"Registry::morph: fn is empty"` for a null `fn`.
- Never mutates the receiver — use [`morph_()`](morph_.md) for the in-place
  form.

## See Also

- [`Registry::morph_`](morph_.md): the in-place counterpart.
- [`Registry::map`](map.md): the whole-`Slot`, every-field counterpart.
- [`Registry::keep`](keep.md): filter a field's values, rather than
  transforming them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minstate.html">
    <span class="label">Previous</span>
    <span class="title">minstate</span>
  </a>
  <a class="next-link" href="morph_.html">
    <span class="label">Next</span>
    <span class="title">morph_</span>
  </a>
</div>
