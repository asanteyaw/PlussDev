<!-- pluss-hand-authored -->

# Axel::nt

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
AxelNoteAccessor Axel::nt() const
```

## What It Does

Returns an [`AxelNoteAccessor`](../../axel_note_accessor_api_reference)
bound to this `Axel`, for applying text (`Note`-kind) operations across
every field of every registry frame in the depth stack.

```{important} Scope with `fields()` first
Like [`cast()`](cast.md) and [`tl()`](tl.md), `nt()` itself takes no
field names — it applies to every field of every frame. Chain
`axel.fields({"..."}).nt()` to scope the effect to one field.
```

## Parameters

`None`

## Returns

`AxelNoteAccessor`
    An accessor exposing the full `Note` text-processing method set
    (`strip()`, `upper()`, `has()`, `count()`, `replace()`, ...) — see
    the [`AxelNoteAccessor` reference](../../axel_note_accessor_api_reference).

## Examples

Example 1. Uppercase the `"city"` field across every frame.

```cpp
auto r0 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})}});
auto r1 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Kumasi"), pluss::make_note("Takoradi")})}});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.fields({"city"}).nt().upper();
std::cout << std::get<pluss::Note>(out.registry(0).fields("city").at(0)).at(0);
```

### Output

```text
ACCRA
```

## Notes

- Every field the accessor touches must be `Note`-kind in every frame
  it's applied to — a non-`Note` field throws whatever the underlying
  `Registry::nt()` call throws for that frame.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`AxelNoteAccessor` API Reference](../../axel_note_accessor_api_reference):
  every method available on the returned accessor.
- [`Axel::fields`](fields.md): narrow to a specific field before
  applying text operations.
- [`Axel::tl`](tl.md): the equivalent entry point for `Timeline`-kind
  fields.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="registries.html">
    <span class="label">Next</span>
    <span class="title">registries</span>
  </a>
</div>
