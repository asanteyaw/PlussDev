<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 15dd2fbfa0d6 -->

# Registry::nt

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
RegistryNoteAccessor Registry::nt() const
```

## What It Does

The entry point into **text (`Note`) operations** — `nt()` binds every field of
the current registry and hands back a `RegistryNoteAccessor`, documented in full
on its own pages (see
[`RegistryNoteAccessor` API Reference](../../registry_note_accessor_api_reference)):
case folding (`upper()`, `lower()`, `titlecase()`), trimming (`strip()`,
`lstrip()`, `rstrip()`), pattern matching (`matches()`, `count()`, `has()`), and
more.

```{important} nt() always binds every field — scope it by projecting first
Same pattern as [`cast()`](cast.md): `nt()` takes no `key` argument and always
operates on every field. To scope text operations to one field, project first:
`rg.fields({"city"}).nt().upper()`.
```

## Parameters

`None`

## Returns

`RegistryNoteAccessor`
    A binder for the current registry's fields — call one of its methods to run
    a text operation across every bound field.

## Examples

Example 1. Uppercase a single, projected field.

```cpp
auto rg = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fields({"city"}).nt().upper();
auto city = out.fields("city");
std::cout << std::get<pluss::Note>(city.at(0)).at(0);
```

### Output

```text
ACCRA
```

## Notes

- `nt()` alone does no work — it only binds the registry's fields; call one of
  the accessor's methods to actually transform text.
- See the important callout above for scoping to specific fields.
- The full accessor surface is documented separately — see
  [`RegistryNoteAccessor` API Reference](../../registry_note_accessor_api_reference).

## See Also

- [`Registry::fields`](fields.md): project down to specific fields before
  running text operations, per the important callout above.
- [`Registry::cast`](cast.md) / [`Registry::tl`](tl.md): the sibling entry
  points for type casting and datetime operations.
- [`Catalog::nt`](../catalog/nt.md): the equivalent entry point on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nstate.html">
    <span class="label">Previous</span>
    <span class="title">nstate</span>
  </a>
  <a class="next-link" href="profiles.html">
    <span class="label">Next</span>
    <span class="title">profiles</span>
  </a>
</div>
