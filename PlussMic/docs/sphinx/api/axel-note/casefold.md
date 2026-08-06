<!-- pluss-hand-authored -->

# AxelNoteAccessor::casefold

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::casefold() const
```

## What It Does

Applies aggressive, locale-independent case folding to every bound
field's text, in every registry frame, and returns a new axel with
those fields replaced. Applies
[`RegistryNoteAccessor::casefold`](../registry-note/casefold.md) to
every frame via [`Axel::map`](../axel/map.md).

```{important} casefold() is not just lower() with a different name
`casefold()` and [`lower()`](lower.md) look similar and often produce
identical output for plain ASCII text, but they're not the same
transform. Case folding is designed for *case-insensitive comparison*,
not display — it can normalize characters that `lower()` leaves alone
(for example, the German `ß` folds toward `ss` under full Unicode case
folding, where lowercasing leaves it as `ß`). If you're about to
compare two text fields for equality regardless of case, prefer
`casefold()` on both sides over `lower()`.
```

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field case-folded, in every frame;
    every unbound field unchanged.

## Examples

Example 1. Case-fold a projected `city` field.

```cpp
auto r0 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra City"), pluss::make_note("TEMA town")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"city"}).nt().casefold();
auto city = out.registry(0).fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " | "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
accra city | tema town
```

## Notes

- See the important callout above for how this differs from
  [`lower()`](lower.md).
- Requires every bound field, in every frame, to be `Note`-kind —
  calling it (or any other method on this accessor) with a non-`Note`
  field bound throws `"Slot::nt: slot kind must be Note"`.
- Bound via [`Axel::nt()`](../axel/nt.md) — remember that entry point
  binds *every* field of every frame unless you project first with
  [`fields(keys)`](../axel/fields.md).

## See Also

- [`AxelNoteAccessor::lower`](lower.md): the plain, non-comparison-oriented
  lowercasing transform.
- [`AxelNoteAccessor::upper`](upper.md) / [`titlecase()`](titlecase.md):
  the other case transforms on this accessor.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
