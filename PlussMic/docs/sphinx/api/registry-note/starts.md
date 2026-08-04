<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: be5160ba6d1c -->

# RegistryNoteAccessor::starts

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::starts(std::string_view prefix) const
```

## What It Does

Checks whether each bound field's text starts with `prefix`, replacing each
`Note` field with a `Bool` `Series` field.

## Parameters

**prefix** (`std::string_view`)
    The literal text to check for at the start of each value. Always a
    literal comparison — there is no regex or case-insensitive mode, unlike
    [`has()`](has.md).

## Returns

`Registry`
    A new registry with every bound field replaced by a `Bool` `Series` field;
    every unbound field unchanged.

## Examples

Example 1. Check which city names start with `"Ac"`.

```cpp
auto rg = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fields({"city"}).nt().starts("Ac");
auto city = out.fields("city");
std::cout << std::boolalpha
          << std::get<pluss::Series>(city.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(city.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

## Notes

- Always a literal, case-sensitive comparison — there's no `TextOptions`
  overload here, unlike [`has()`](has.md) or [`count()`](count.md).
- Changes each bound field's kind from `Note` to `Series` (`Bool`).
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`RegistryNoteAccessor::ends`](ends.md): the mirror check at the end of the
  text instead of the beginning.
- [`RegistryNoteAccessor::has`](has.md): a general substring/pattern search
  anywhere in the text, with case-insensitive and regex options.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html">
    <span class="label">Previous</span>
    <span class="title">rstrip</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
