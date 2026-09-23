<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 317849c51981 -->

# RegistryNoteAccessor::length

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::length() const
```

## What It Does

Measures the character length of each bound field's text, replacing each
`Note` field with an `Int64` `Series` field of lengths.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of character counts; every unbound field unchanged.

## Examples

Example 1. Measure city name lengths.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Kumasi")})},
});

auto out = rg.fields({"city"}).nt().length();
auto city = out.fields("city");
std::cout << std::get<pluss::Series>(city.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(city.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
5 6
```

## Notes

- Changes each bound field's kind from `Note` to `Series` (`Int64`).
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.
- Bound via [`Registry::nt()`](../registry/nt.md), which binds every field of
  the registry unless you project first with
  [`fields(keys)`](../registry/fields.md).

## See Also

- [`RegistryNoteAccessor::chars`](chars.md): use a length result to compute
  slice bounds relative to the end of the text.
- [`RegistryNoteAccessor::count`](count.md): the other method on this
  accessor that turns text into an `Int64` `Series`.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

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
