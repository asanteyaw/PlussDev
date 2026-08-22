<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3d47a4b008d1 -->

# Registry::cast

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
RegistryCastAccessor Registry::cast() const
RegistryCastAccessor Registry::cast(const pluss::table::ThreadCastOptions& options) const
```

## What It Does

The entry point into **type casting** — `cast()` binds every field of the
current registry and hands back a `RegistryCastAccessor`, documented in full on
its own pages (see
[`RegistryCastAccessor` API Reference](../../registry_cast_accessor_api_reference)):
`series()`, `note()`, `timeline()`, `timeindex()`, one per target kind. Each
accessor method returns a new `Registry` with every bound field cast to that
kind.

```{important} cast() always binds every field — scope it by projecting first
Unlike the field-scoped mutation methods (`push(key, ...)`, `keep(key, ...)`,
`morph(key, ...)`, `concat(key, ...)`), `cast()` takes no `key` argument — it
always operates on **every** field of the registry it's called on. To cast only
some fields, call `cast()` on a narrower, projected registry instead:
`rg.fields({"id"}).cast().note()`.
```

## Parameters

**options** (`const pluss::table::ThreadCastOptions&`, optional)
    Casting behavior options (e.g. how parse failures are handled), shared with
    the table model's cast machinery.

## Returns

`RegistryCastAccessor`
    A binder for the current registry's fields — call one of its methods to cast
    every bound field to that kind.

## Examples

Example 1. Cast every field of a registry to `Note`-kind.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
});

auto out = rg.cast().note();
std::cout << out.fields("id").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    "Note[numel=1]",
    "Note[numel=1]"
  ]
}
```

Example 2. Cast only one field by projecting first.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"population", pluss::scribble::Slot({pluss::make_series(100), pluss::make_series(200)})},
});

auto id_as_note = rg.fields({"id"}).cast().note();
std::cout << id_as_note.fields("id").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    "Note[numel=1]",
    "Note[numel=1]"
  ]
}
```

## Notes

- `cast()` alone does no work — it only binds the registry's fields; call one of
  the accessor's methods to actually cast.
- See the important callout above for scoping to specific fields.
- The full accessor surface is documented separately — see
  [`RegistryCastAccessor` API Reference](../../registry_cast_accessor_api_reference).

## See Also

- [`Registry::fields`](fields.md): project down to specific fields before
  casting, per the important callout above.
- [`Registry::nt`](nt.md) / [`Registry::tl`](tl.md): the sibling entry points
  for text and datetime operations, following the same every-field binding
  pattern.
- [`Catalog::cast`](../catalog/cast.md): the equivalent entry point on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="begin.html">
    <span class="label">Previous</span>
    <span class="title">begin</span>
  </a>
  <a class="next-link" href="clear.html">
    <span class="label">Next</span>
    <span class="title">clear</span>
  </a>
</div>
