<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e82559b3657d -->

# AxelNoteAccessor::ends

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::ends(std::string_view suffix) const
```

## What It Does

Checks whether each bound field's text ends with `suffix`, in every
registry frame, replacing each `Note` field with a `Bool` `Series`
field. Applies
[`RegistryNoteAccessor::ends`](../registry-note/ends.md) to every frame
via [`Axel::map`](../axel/map.md).

## Parameters

**suffix** (`std::string_view`)
    The literal text to check for at the end of each value. Always a
    literal comparison — there is no regex or case-insensitive mode,
    unlike [`has()`](has.md).

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Bool` `Series` field; every unbound field unchanged.

## Examples

Example 1. Check which city names end in `"ma"`.

```cpp
auto r0 = pluss::scribble::Registry({{"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"city"}).nt().ends("ma");
auto city = out.registry(0).fields("city");
std::cout << std::boolalpha
          << std::get<pluss::Series>(city.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(city.at(1)).to_vec<bool>()[0];
```

### Output

```text
false true
```

## Notes

- Always a literal, case-sensitive comparison — there's no
  `TextOptions` overload here, unlike [`has()`](has.md) or
  [`count()`](count.md).
- Changes each bound field's kind from `Note` to `Series` (`Bool`).
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.

## See Also

- [`AxelNoteAccessor::starts`](starts.md): the mirror check at the
  beginning of the text instead of the end.
- [`AxelNoteAccessor::has`](has.md): a general substring/pattern search
  anywhere in the text, with case-insensitive and regex options.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="fullmatch.html">
    <span class="label">Next</span>
    <span class="title">fullmatch</span>
  </a>
</div>
