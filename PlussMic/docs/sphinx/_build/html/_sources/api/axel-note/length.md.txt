<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 62fb6cabbf03 -->

# AxelNoteAccessor::length

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::length() const
```

## What It Does

Measures the character length of each bound field's text, in every
registry frame, replacing each `Note` field with an `Int64` `Series`
field of lengths. Applies
[`RegistryNoteAccessor::length`](../registry-note/length.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by an
    `Int64` `Series` field of character counts; every unbound field
    unchanged.

## Examples

Example 1. Measure city name lengths.

```cpp
auto r0 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Kumasi")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"city"}).nt().length();
auto city = out.registry(0).fields("city");
std::cout << std::get<pluss::Series>(city.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(city.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
5 6
```

## Notes

- Changes each bound field's kind from `Note` to `Series` (`Int64`).
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.
- Bound via [`Axel::nt()`](../axel/nt.md), which binds every field of
  every frame unless you project first with
  [`fields(keys)`](../axel/fields.md).

## See Also

- [`AxelNoteAccessor::chars`](chars.md): use a length result to compute
  slice bounds relative to the end of the text.
- [`AxelNoteAccessor::count`](count.md): the other method on this
  accessor that turns text into an `Int64` `Series`.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>
