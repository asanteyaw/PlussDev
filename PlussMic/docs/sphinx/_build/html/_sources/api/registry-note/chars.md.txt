<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 08fc627adb7c -->

# RegistryNoteAccessor::chars

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::chars(std::int64_t start,
                                     std::optional<std::int64_t> stop = std::nullopt,
                                     std::int64_t step = 1) const
```

## What It Does

Slices each bound field's text by character position — Python-style
`text[start:stop:step]` semantics, including negative indices counting from
the end.

## Parameters

**start** (`std::int64_t`)
    The starting character position of the slice, inclusive. Negative values
    count from the end of the text (`-1` is the last character).

**stop** (`std::optional<std::int64_t>`, defaults to `std::nullopt`)
    The ending character position, exclusive. `std::nullopt` (the default)
    slices through to the end of the text.

**step** (`std::int64_t`, defaults to `1`)
    The stride between characters kept in the slice.

## Returns

`Registry`
    A new registry with every bound field replaced by the sliced text; every
    unbound field unchanged.

## Examples

Example 1. Take the first two characters of each value.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Kumasi")})},
});

auto out = rg.fields({"city"}).nt().chars(0, 2);
auto city = out.fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
Ac Ku
```

Example 2. A negative `start` slices from the end — the last two characters
of each value.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra")})},
});

auto out = rg.fields({"city"}).nt().chars(-2);
std::cout << std::get<pluss::Note>(out.fields("city").at(0)).at(0);
```

### Output

```text
ra
```

Example 3. `step` keeps every Nth character starting at `start`, with no
`stop` bound (`std::nullopt` runs to the end).

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra")})},
});

auto out = rg.fields({"city"}).nt().chars(0, std::nullopt, 2);
std::cout << std::get<pluss::Note>(out.fields("city").at(0)).at(0);
```

### Output

```text
Aca
```

## Notes

- `stop` defaults to `std::nullopt` (slice to the end), not to `start`'s
  value — passing only `start` slices the entire remaining tail, not an empty
  string.
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.
- Bound via [`Registry::nt()`](../registry/nt.md), which binds every field of
  the registry unless you project first with
  [`fields(keys)`](../registry/fields.md).

## See Also

- [`RegistryNoteAccessor::length`](length.md): get each value's character
  count, useful for computing slice bounds relative to the end.
- [`RegistryNoteAccessor::strip`](strip.md): trim from the edges instead of
  slicing by position.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
