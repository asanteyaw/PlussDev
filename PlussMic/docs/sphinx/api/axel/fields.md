<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b6af95492a76 -->

# Axel::fields

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::fields(std::span<const std::string_view> keys) const
Axel Axel::fields(std::initializer_list<std::string_view> keys) const
template <pluss::detail::StringViewNameRange NameRange>
Axel Axel::fields(const NameRange& keys) const
```

## What It Does

Narrows every registry frame in the depth stack down to only the named
field(s), returning a new `Axel`. Internally, this calls
[`map()`](map.md) with `Registry::fields(keys)` applied to each frame —
the same narrowing `Registry::fields()` performs, just broadcast across
every depth index at once.

This is the idiom used to scope every accessor entry point
([`cast()`](cast.md), [`nt()`](nt.md), [`tl()`](tl.md)) to a specific
field before calling one of its methods — see those pages' examples,
which all start with `axel.fields({"..."}).accessor().method()`.

## Parameters

**keys** (`std::span<const std::string_view>`, `std::initializer_list<std::string_view>`, or any range of string-view-like names)
    One or more field names to keep in every frame. Cannot be empty.
    Field names must be non-empty and unique within the call — duplicate
    or blank names throw.

## Returns

`Axel`
    A new axel with every frame narrowed to `keys`, same depth as the
    original.

## Examples

Example 1. Narrow a two-field, two-frame `Axel` down to just `"ids"`.

```cpp
auto r0 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto r1 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Kumasi"), pluss::make_note("Takoradi")})},
});
auto axel = pluss::scribble::Axel({r0, r1});

auto narrowed = axel.fields({"ids"});
std::cout << narrowed.registry(0).nfields();
```

### Output

```text
1
```

Example 2. Calling `fields()` with no names throws.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
});
try {
  std::vector<std::string_view> empty_keys;
  axel.fields(std::span<const std::string_view>(empty_keys));
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Axel::fields: at least one field is required
```

## Notes

- `keys` cannot be empty (`"Axel::fields: at least one field is
  required"`), each name must be non-empty (`"Axel: field cannot be
  empty"`), and names cannot repeat within one call
  (`"Axel: duplicate field 'x'"`).
- Because this delegates to `Registry::fields(keys)` per frame, a field
  missing from any individual frame throws whatever
  `Registry::fields()` throws for that frame — every frame does not
  need an identical schema for other `Axel` methods, but `fields()`
  requires the selected names to exist in *every* frame it's applied
  to.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Axel::fieldaxis`](fieldaxis.md): select fields by pattern or `Slot`
  kind instead of by exact name.
- [`Axel::cast`](cast.md), [`Axel::nt`](nt.md), [`Axel::tl`](tl.md):
  the three accessor entry points typically chained after `fields()` to
  scope their effect to one field.
- [`Axel::drop`](drop.md): the complementary operation, removing named
  fields instead of keeping only them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fieldaxis.html">
    <span class="label">Previous</span>
    <span class="title">fieldaxis</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
