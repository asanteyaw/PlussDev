<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ce79a5dc7a69 -->

# RegistryNoteAccessor::casefold

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::casefold() const
```

## What It Does

Applies aggressive, locale-independent case folding to every bound field's
text, and returns a new registry with those fields replaced.

```{important} casefold() is not just lower() with a different name
`casefold()` and [`lower()`](lower.md) look similar and often produce identical
output for plain ASCII text, but they're not the same transform. Case folding is
designed for *case-insensitive comparison*, not display — it can normalize
characters that `lower()` leaves alone (for example, the German `ß` folds
toward `ss` under full Unicode case folding, where lowercasing leaves it as
`ß`). If you're about to compare two text fields for equality regardless of
case, prefer `casefold()` on both sides over `lower()`.
```

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field case-folded; every unbound field
    unchanged.

## Examples

Example 1. Case-fold a projected `city` field.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra City"), pluss::make_note("TEMA town")})},
});

auto out = rg.fields({"city"}).nt().casefold();
auto city = out.fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " | "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
accra city | tema town
```

Example 2. `nt()` with no projection binds *every* field of the registry —
scope it with [`fields(keys)`](../registry/fields.md) first if only some
fields should be affected.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra")})},
    {"country", pluss::scribble::Slot({pluss::make_note("GHANA")})},
});

auto out = rg.nt().casefold();
std::cout << std::get<pluss::Note>(out.fields("city").at(0)).at(0) << " "
          << std::get<pluss::Note>(out.fields("country").at(0)).at(0);
```

### Output

```text
accra ghana
```

## Notes

- See the important callout above for how this differs from
  [`lower()`](lower.md).
- Requires every bound field to be `Note`-kind — calling it (or any other
  method on this accessor) with a non-`Note` field bound throws
  `"Slot::nt: slot kind must be Note"`.
- Bound via [`Registry::nt()`](../registry/nt.md) — remember that entry point
  binds *every* field of the registry it's called on unless you project first,
  per Example 2.

## See Also

- [`RegistryNoteAccessor::lower`](lower.md): the plain, non-comparison-oriented
  lowercasing transform.
- [`RegistryNoteAccessor::upper`](upper.md) / [`titlecase()`](titlecase.md):
  the other case transforms on this accessor.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
