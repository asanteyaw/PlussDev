<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f2a90b5490dd -->

# Registry::get

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::optional<Slot> Registry::get(std::string_view key) const
```

## What It Does

Fetches a field's `Slot` by name, returning `std::nullopt` instead of throwing when
the name doesn't exist. This is the safe way to test for a field's presence and
retrieve it in one call — use [`has()`](has.md) if you only need the presence check,
or [`fields(key)`](fields.md) if you're confident the field exists and want the
`Slot` directly (throwing otherwise).

## Parameters

**key** (`std::string_view`)
    The field name to look up.

## Returns

`std::optional<Slot>`
    The field's `Slot` if `key` exists; `std::nullopt` otherwise. Requires a defined
    registry; throws on an undefined one.

## Examples

Example 1. An existing key returns an engaged `optional`.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto slot = rg.get("city");
std::cout << std::boolalpha << slot.has_value();
```

### Output

```text
true
```

Example 2. A missing key returns `std::nullopt` — no throw.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

auto slot = rg.get("zzz");
std::cout << std::boolalpha << slot.has_value();
```

### Output

```text
false
```

## Notes

- Never throws on a missing key — unlike `fields(key)`, which throws.
- Key comparison is exact and case-sensitive, same as [`has()`](has.md).
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::has`](has.md): a pure presence check, without fetching the `Slot`.
- [`Registry::fields`](fields.md): fetch the `Slot` directly (single-key overload),
  throwing if the field is missing.
- [`Catalog::get`](../catalog/get.md): the equivalent lookup on the kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
