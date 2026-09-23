<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2c6dd874c25e -->

# Registry::fillnull

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::fillnull(double value) const
Registry Registry::fillnull(std::string_view value) const
Registry Registry::fillnull(const char* value) const
Registry Registry::fillnull(const pluss::Timeline& value) const
Registry Registry::fillnull(const pluss::TimeIndex& value) const
```

## What It Does

Returns a new registry with null values replaced by `value`, applying
[`Slot::fillnull(value)`](../slot/fillnull.md) to **every** field via
[`map()`](map.md) — one overload per fill-value kind (`double` for numeric
fields, `std::string_view`/`const char*` for text fields, `Timeline`/`TimeIndex`
for datetime fields).

```{important} Every field is filled, and every field's kind must match — even fields with no nulls
Because `fillnull()` applies to every field unconditionally via `map()`, calling
`fillnull(double value)` on a registry that has **any** non-numeric field throws
— even if that field has zero null values. `Slot::fillnull(double)` requires
every *existing, non-null* value in the slot to already be numeric-compatible
before it will fill anything; a `Note`-kind field fails that check outright. The
same applies symmetrically to the other overloads: `fillnull(string_view)`
requires every field to be text-compatible, and so on. There is no way to scope
`fillnull()` to specific fields at the `Registry` level — for a mixed-kind
registry, fetch the field via [`fields(key)`](fields.md), call `fillnull()` on
just that `Slot`, and reinsert with [`set()`](set.md).
```

## Parameters

**value** (`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` / `const pluss::TimeIndex&`)
    The replacement for null values, in the kind matching the overload selected.

## Returns

`Registry`
    A new registry with every field's null values replaced by `value`.

## Examples

Example 1. On a registry where every field is numeric, `fillnull(double)`
succeeds and fills the null.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_series(3)})},
    {"population", pluss::scribble::Slot({pluss::make_series(100), pluss::make_series(200), pluss::make_series(300)})},
});

auto out = rg.fillnull(99.0);
std::cout << out.fields("id").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Series[int64]",
    "Series[float64]",
    "Series[int64]"
  ]
}
```

Example 2. On a registry with a mixed numeric field *and* a `Note` field,
`fillnull(double)` throws — even though the `Note` field itself has no nulls to
fill.

```cpp
auto mixed = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

try {
  auto out = mixed.fillnull(99.0);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slot::fillnull(double): non-null Slot values must match fill payload type
```

Example 3. On a `Note`-only registry, `fillnull(string_view)` (via the
`const char*` overload) fills the null with the given text.

```cpp
auto notes = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_note("Kumasi")})},
});

auto out = notes.fillnull("Unknown");
std::cout << out.fields("city").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Note[numel=1]",
    "Note[numel=1]",
    "Note[numel=1]"
  ]
}
```

## Notes

- See the important callout above — this is the single most surprising thing
  about `Registry::fillnull()`: it can throw on fields that don't even have
  nulls, because it applies unconditionally to every field.
- Filling a null in an integer `Series` field with a `double` promotes that
  slot's numeric entries to `Series[float64]` — see Example 1.
- Never mutates the receiver — always returns a new `Registry`.

## See Also

- [`Registry::dropnull`](dropnull.md): removes null values instead of filling
  them, and (unlike `fillnull()`) never throws over kind mismatches.
- [`Registry::isnull`](isnull.md): identify which values are null before
  deciding how to fill them.
- [`Registry::fields`](fields.md) / [`Registry::set`](set.md): scope a fill to a
  single field on a mixed-kind registry, avoiding the important callout above.
- [`Catalog::fillnull`](../catalog/fillnull.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fields.html">
    <span class="label">Previous</span>
    <span class="title">fields</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
