<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3e0f1b35e11a -->

# AxelNoteAccessor::lower

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::lower() const
```

## What It Does

Lowercases every bound field's text, in every registry frame, and
returns a new axel with those fields replaced. Applies
[`RegistryNoteAccessor::lower`](../registry-note/lower.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field lowercased, in every frame; every
    unbound field unchanged.

## Examples

Example 1. Lowercase a projected `city` field.

```cpp
auto r0 = pluss::scribble::Registry({{"city", pluss::scribble::Slot({pluss::make_note("Accra City"), pluss::make_note("TEMA town")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"city"}).nt().lower();
auto city = out.registry(0).fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " | "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
accra city | tema town
```

## Notes

- For case-insensitive *comparison* rather than display, prefer
  [`casefold()`](casefold.md) — see its important callout for why the
  two aren't interchangeable.
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.
- Bound via [`Axel::nt()`](../axel/nt.md), which binds every field of
  every frame unless you project first with
  [`fields(keys)`](../axel/fields.md).

## See Also

- [`AxelNoteAccessor::upper`](upper.md): the inverse transform.
- [`AxelNoteAccessor::casefold`](casefold.md): the comparison-oriented
  case-folding transform.
- [`AxelNoteAccessor::titlecase`](titlecase.md): capitalizes the first
  letter of each word instead of lowercasing everything.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

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
