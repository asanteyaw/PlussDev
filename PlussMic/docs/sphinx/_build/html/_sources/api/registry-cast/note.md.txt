<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f24df731c89b -->

# RegistryCastAccessor::note

[Back to RegistryCastAccessor API Reference](../../registry_cast_accessor_api_reference)

## Signature

```cpp
Registry RegistryCastAccessor::note() const
```

## What It Does

Casts every bound field's values to `Note`-kind (text) and returns a new
registry with those fields replaced. Unlike casting toward `Series`,
`Note`-conversion always succeeds — every domain kind has a printable text
form.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field cast to `Note`-kind; every unbound
    field unchanged.

## Examples

Example 1. Cast a numeric `id` field to text.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
});

auto out = rg.fields({"id"}).cast().note();
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

## Notes

- Bound via [`Registry::cast()`](../registry/cast.md) — remember that entry
  point binds *every* field of the registry it's called on, so scope it with
  [`fields(keys)`](../registry/fields.md) first if you only want to cast some
  fields.
- Unlike [`series()`](series.md), casting to `Note` doesn't have a
  silent-failure mode — any value can be rendered as text.

## See Also

- [`RegistryCastAccessor::series`](series.md): the inverse direction, with a
  silent-null failure mode for unparseable text.
- [`Registry::cast`](../registry/cast.md): the entry point this accessor is
  reached from.
- [`Slot::cast`](../slot/cast.md): the per-field casting this method applies
  across every bound field.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
