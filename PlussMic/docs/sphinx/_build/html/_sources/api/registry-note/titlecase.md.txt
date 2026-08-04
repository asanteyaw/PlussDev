<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 94ab970fb8f0 -->

# RegistryNoteAccessor::titlecase

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::titlecase() const
```

## What It Does

Capitalizes the first letter of each word in every bound field's text and
lowercases the rest, returning a new registry with those fields replaced.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field titlecased; every unbound field
    unchanged.

## Examples

Example 1. Titlecase a mixed-case field.

```cpp
auto rg = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra City"), pluss::make_note("TEMA town")})},
});

auto out = rg.fields({"city"}).nt().titlecase();
auto city = out.fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " | "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
Accra City | Tema Town
```

## Notes

- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.
- Bound via [`Registry::nt()`](../registry/nt.md), which binds every field of
  the registry unless you project first with
  [`fields(keys)`](../registry/fields.md).

## See Also

- [`RegistryNoteAccessor::lower`](lower.md) / [`upper()`](upper.md): the
  simpler, whole-string case transforms.
- [`RegistryNoteAccessor::casefold`](casefold.md): the comparison-oriented
  case-folding transform.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
