<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 981c70dae867 -->

# AxelNoteAccessor::chars

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::chars(std::int64_t start,
                             std::optional<std::int64_t> stop = std::nullopt,
                             std::int64_t step = 1) const
```

## What It Does

Slices each bound field's text by character position, in every registry
frame — Python-style `text[start:stop:step]` semantics, including
negative indices counting from the end. Applies
[`RegistryNoteAccessor::chars`](../registry-note/chars.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

**start** (`std::int64_t`)
    The starting character position of the slice, inclusive. Negative
    values count from the end of the text (`-1` is the last character).

**stop** (`std::optional<std::int64_t>`, defaults to `std::nullopt`)
    The ending character position, exclusive. `std::nullopt` (the
    default) slices through to the end of the text.

**step** (`std::int64_t`, defaults to `1`)
    The stride between characters kept in the slice.

## Returns

`Axel`
    A new axel with every bound field replaced by the sliced text, in
    every frame; every unbound field unchanged.

## Examples

Example 1. Take the first two characters of each value.

```cpp
auto r0 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Kumasi")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"city"}).nt().chars(0, 2);
auto city = out.registry(0).fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
Ac Ku
```

Example 2. A negative `start` slices from the end — the last two
characters of each value.

```cpp
auto r0 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"city"}).nt().chars(-2);
std::cout << std::get<pluss::Note>(out.registry(0).fields("city").at(0)).at(0);
```

### Output

```text
ra
```

Example 3. `step` keeps every Nth character starting at `start`, with
no `stop` bound (`std::nullopt` runs to the end).

```cpp
auto r0 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"city"}).nt().chars(0, std::nullopt, 2);
std::cout << std::get<pluss::Note>(out.registry(0).fields("city").at(0)).at(0);
```

### Output

```text
Aca
```

## Notes

- `stop` defaults to `std::nullopt` (slice to the end), not to
  `start`'s value — passing only `start` slices the entire remaining
  tail, not an empty string.
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.
- Bound via [`Axel::nt()`](../axel/nt.md), which binds every field of
  every frame unless you project first with
  [`fields(keys)`](../axel/fields.md).

## See Also

- [`AxelNoteAccessor::length`](length.md): get each value's character
  count, useful for computing slice bounds relative to the end.
- [`AxelNoteAccessor::strip`](strip.md): trim from the edges instead of
  slicing by position.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

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
