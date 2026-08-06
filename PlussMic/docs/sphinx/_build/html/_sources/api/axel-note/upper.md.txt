<!-- pluss-hand-authored -->

# AxelNoteAccessor::upper

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::upper() const
```

## What It Does

Uppercases every bound field's text, in every registry frame, and
returns a new axel with those fields replaced. Applies
[`RegistryNoteAccessor::upper`](../registry-note/upper.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field uppercased, in every frame; every
    unbound field unchanged.

## Examples

Example 1. Uppercase a single, projected field.

```cpp
auto r0 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"city"}).nt().upper();
auto city = out.registry(0).fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0) << " "
          << std::get<pluss::Note>(city.at(1)).at(0);
```

### Output

```text
ACCRA TEMA
```

## Notes

- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.
- Bound via [`Axel::nt()`](../axel/nt.md), which binds every field of
  every frame unless you project first with
  [`fields(keys)`](../axel/fields.md).

## See Also

- [`AxelNoteAccessor::lower`](lower.md): the inverse transform.
- [`AxelNoteAccessor::titlecase`](titlecase.md): capitalizes only the
  first letter of each word instead of uppercasing everything.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="titlecase.html">
    <span class="label">Previous</span>
    <span class="title">titlecase</span>
  </a>
</div>
