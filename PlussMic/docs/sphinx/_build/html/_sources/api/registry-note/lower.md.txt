<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 01a36a63c0be -->

# RegistryNoteAccessor::lower

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::lower() const
```

## What It Does

Lowercases every bound field's text and returns a new registry with those
fields replaced.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field lowercased; every unbound field
    unchanged.

## Examples

Example 1. Lowercase a projected `city` field.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra City"), pluss::make_note("TEMA town")})},
});

auto out = rg.fields({"city"}).nt().lower();
auto city = out.fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " | "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
accra city | tema town
```

## Notes

- For case-insensitive *comparison* rather than display, prefer
  [`casefold()`](casefold.md) — see its important callout for why the two
  aren't interchangeable.
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.
- Bound via [`Registry::nt()`](../registry/nt.md), which binds every field of
  the registry unless you project first with
  [`fields(keys)`](../registry/fields.md).

## See Also

- [`RegistryNoteAccessor::upper`](upper.md): the inverse transform.
- [`RegistryNoteAccessor::casefold`](casefold.md): the comparison-oriented
  case-folding transform.
- [`RegistryNoteAccessor::titlecase`](titlecase.md): capitalizes the first
  letter of each word instead of lowercasing everything.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="length.html">
    <span class="label">Previous</span>
    <span class="title">length</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>
