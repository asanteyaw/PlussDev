<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6d1615166b92 -->

# RegistryNoteAccessor::upper

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::upper() const
```

## What It Does

Uppercases every bound field's text and returns a new registry with those
fields replaced.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field uppercased; every unbound field
    unchanged.

## Examples

Example 1. Uppercase a single, projected field.

```cpp
auto rg = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fields({"city"}).nt().upper();
auto city = out.fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
ACCRA TEMA
```

Example 2. `nt()` with no projection binds every field of the registry.

```cpp
auto rg = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra")})},
    {"country", pluss::bundle::Slot({pluss::make_note("Ghana")})},
});

auto out = rg.nt().upper();
std::cout << std::get<pluss::Note>(out.fields("city").at(0)).at(0) << " "
          << std::get<pluss::Note>(out.fields("country").at(0)).at(0);
```

### Output

```text
ACCRA GHANA
```

## Notes

- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.
- Bound via [`Registry::nt()`](../registry/nt.md), which binds every field of
  the registry unless you project first with
  [`fields(keys)`](../registry/fields.md), per Example 2.

## See Also

- [`RegistryNoteAccessor::lower`](lower.md): the inverse transform.
- [`RegistryNoteAccessor::titlecase`](titlecase.md): capitalizes only the
  first letter of each word instead of uppercasing everything.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="titlecase.html">
    <span class="label">Previous</span>
    <span class="title">titlecase</span>
  </a>
</div>
