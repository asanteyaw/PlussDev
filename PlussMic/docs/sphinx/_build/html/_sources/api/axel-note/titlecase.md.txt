<!-- pluss-hand-authored -->

# AxelNoteAccessor::titlecase

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::titlecase() const
```

## What It Does

Capitalizes the first letter of each word in every bound field's text
and lowercases the rest, in every registry frame, returning a new axel
with those fields replaced. Applies
[`RegistryNoteAccessor::titlecase`](../registry-note/titlecase.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field titlecased, in every frame; every
    unbound field unchanged.

## Examples

Example 1. Titlecase a mixed-case field.

```cpp
auto r0 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra City"), pluss::make_note("TEMA town")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"city"}).nt().titlecase();
auto city = out.registry(0).fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " | "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
Accra City | Tema Town
```

## Notes

- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.
- Bound via [`Axel::nt()`](../axel/nt.md), which binds every field of
  every frame unless you project first with
  [`fields(keys)`](../axel/fields.md).

## See Also

- [`AxelNoteAccessor::lower`](lower.md) / [`upper()`](upper.md): the
  simpler, whole-string case transforms.
- [`AxelNoteAccessor::casefold`](casefold.md): the comparison-oriented
  case-folding transform.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

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
