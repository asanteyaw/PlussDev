<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 97d62e3e518d -->

# Registry::set_

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry& Registry::set_(std::string_view key, const Slot& value)
Registry& Registry::set_(std::initializer_list<std::pair<std::string_view, Slot>> items)
Registry& Registry::set_(std::initializer_list<std::pair<std::string, Slot>> items)
```

## What It Does

The in-place counterpart to [`set()`](set.md) — sets the named field(s) on this
registry directly, upserting (replacing an existing field or adding a new one).

## Parameters

**key** (`std::string_view`)
    A single field name to set (single-item overload).

**value** (`const Slot&`)
    The new `Slot` for that field. Must be defined.

**items** (`std::initializer_list<std::pair<std::string_view, Slot>>` / `std::initializer_list<std::pair<std::string, Slot>>`)
    Several `{key, Slot}` pairs to set at once.

## Returns

`Registry&`
    A reference to this registry, now holding the updated field(s).

## Examples

Example 1. Overwrite a field in place.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

rg.set_("id", pluss::scribble::Slot({pluss::make_series(42)}));
auto ids = rg.fields("id");
std::cout << std::get<pluss::Series>(ids.at(0)).host<std::int64_t>()[0];
```

### Output

```text
42
```

## Notes

- Upsert semantics, same as [`set()`](set.md): unknown keys are added rather
  than throwing.
- Mutates the registry it's called on and returns `*this` by reference.

## See Also

- [`Registry::set`](set.md): the non-mutating counterpart.
- [`Registry::morph_`](morph_.md): rewrite individual *values* within a field in
  place, rather than replacing the field's whole `Slot`.
- [`Catalog::set_`](../catalog/set_.md): the equivalent operation on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>
